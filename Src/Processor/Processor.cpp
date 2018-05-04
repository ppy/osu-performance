#include <PrecompiledHeader.h>

#include "Processor.h"

#include "Standard/StandardScore.h"
#include "CatchTheBeat/CatchTheBeatScore.h"
#include "Taiko/TaikoScore.h"
#include "Mania/ManiaScore.h"

#include "../Shared/Network/UpdateBatch.h"

using namespace std::chrono;

PP_NAMESPACE_BEGIN

const std::array<const std::string, NumGamemodes> Processor::s_gamemodeSuffixes =
{
	"",
	"_taiko",
	"_fruits",
	"_mania",
};

const std::array<const std::string, NumGamemodes> Processor::s_gamemodeNames =
{
	"osu!",
	"Taiko",
	"Catch the Beat",
	"osu!mania",
};

const std::array<const std::string, NumGamemodes> Processor::s_gamemodeTags =
{
	"osu",
	"taiko",
	"catch_the_beat",
	"osu_mania",
};

Processor::Processor(EGamemode gamemode, const std::string& configFile)
: _gamemode{gamemode}, _config{configFile}, _dataDog{"127.0.0.1", 8125}
{
	Log(None,           "---------------------------------------------------");
	Log(None, StrFormat("---- pp processor for gamemode {0}", GamemodeName(gamemode)));
	Log(None,           "---------------------------------------------------");

	_dataDog.Increment("osu.pp.startups", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

	_pDB = newDBConnectionMaster();
	_pDBSlave = newDBConnectionSlave();

	queryBeatmapBlacklist();
	queryBeatmapDifficultyAttributes();
	queryAllBeatmapDifficulties();
}

Processor::~Processor()
{
	Log(Info, "Shutting down.");
}

void Processor::MonitorNewScores()
{
	_lastScorePollTime = steady_clock::now();
	_lastBeatmapSetPollTime = steady_clock::now();

	_currentScoreId = retrieveCount(*_pDB, lastScoreIdKey());

	auto res = _pDBSlave->Query("SELECT MAX(`approved_date`) FROM `osu_beatmapsets` WHERE 1");

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Couldn't find maximum approved date.");

	_lastApprovedDate = res.String(0);

	std::thread beatmapPollThread{[this]()
	{
		while (!_shallShutdown)
		{
			if (steady_clock::now() - _lastBeatmapSetPollTime > milliseconds{_config.DifficultyUpdateInterval})
				pollAndProcessNewBeatmapSets();
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}};

	std::thread scorePollThread{[this]()
	{
		while (!_shallShutdown)
		{
			if (steady_clock::now() - _lastScorePollTime > milliseconds{_config.ScoreUpdateInterval})
				pollAndProcessNewScores();
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}};

	scorePollThread.join();
	beatmapPollThread.join();
}

void Processor::ProcessAllUsers(bool reProcess, u32 numThreads)
{
	ThreadPool threadPool{numThreads};
	std::vector<std::shared_ptr<DatabaseConnection>> databaseConnections;
	std::vector<UpdateBatch> newUsersBatches;
	std::vector<UpdateBatch> newScoresBatches;

	for (u32 i = 0; i < numThreads; ++i)
	{
		databaseConnections.push_back(newDBConnectionMaster());

		newUsersBatches.emplace_back(databaseConnections[i], 10000);
		newScoresBatches.emplace_back(databaseConnections[i], 10000);
	}

	static const s32 userIdStep = 10000;

	s64 begin; // Will be initialized in the next few lines
	if (reProcess)
	{
		begin = 0;

		// Make sure in case of a restart we still do the full process, even if we didn't trigger a store before
		storeCount(*_pDB, lastUserIdKey(), begin);
	}
	else
		begin = retrieveCount(*_pDB, lastUserIdKey());

	// We're done, nothing to reprocess
	if (begin == -1)
		return;

	Log(Info, StrFormat("Processing all users starting with ID {0}.", begin));

	auto res = _pDBSlave->Query(StrFormat(
		"SELECT MAX(`user_id`),COUNT(`user_id`) FROM `osu_user_stats{0}` WHERE `user_id`>={1}",
		GamemodeSuffix(_gamemode), begin
	));

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Could not find user ID stats.");

	const s64 maxUserId = res.S64(0);
	const s64 numUsers = res.S64(1);

	s64 numUsersProcessed = 0;
	u32 currentConnection = 0;

	// We will break out as soon as there are no more results
	while (begin <= maxUserId)
	{
		s64 end = begin + userIdStep;

		res = _pDBSlave->Query(StrFormat(
			"SELECT "
			"`user_id`"
			"FROM `osu_user_stats{0}` "
			"WHERE `user_id` BETWEEN {1} AND {2}",
			GamemodeSuffix(_gamemode), begin, end
		));

		while (res.NextRow())
		{
			s64 userId = res.S64(0);

			threadPool.EnqueueTask(
				[this, userId, currentConnection, &databaseConnections, &newUsersBatches, &newScoresBatches]()
				{
					processSingleUser(
						0, // We want to update _all_ scores
						*databaseConnections[currentConnection],
						newUsersBatches[currentConnection],
						newScoresBatches[currentConnection],
						userId
					);
				}
			);

			currentConnection = (currentConnection + 1) % numThreads;

			// Shut down when requested!
			if (_shallShutdown)
				return;
		}

		begin += userIdStep;
		numUsersProcessed += res.NumRows();

		LogProgress(numUsersProcessed, numUsers);

		u32 numPendingQueries = 0;

		do
		{
			numPendingQueries = 0;
			for (auto& pDBConn : databaseConnections)
				numPendingQueries += (u32)pDBConn->NumPendingQueries();

			_dataDog.Gauge("osu.pp.db.pending_queries", numPendingQueries,
			{
				StrFormat("mode:{0}", GamemodeTag(_gamemode)),
				"connection:background",
			}, 0.01f);

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		while (threadPool.GetNumTasksInSystem() > 0 || numPendingQueries > 0);

		// Update our user_id counter
		storeCount(*_pDB, lastUserIdKey(), begin);
	}

	Log(Success, StrFormat("Processed all {0} users.", numUsers));
}

void Processor::ProcessUsers(const std::vector<std::string>& userNames)
{
	std::vector<s64> userIds;
	for (const auto& name : userNames)
	{
		s64 id = xtoi64(name.c_str());
		if (id == 0) {
			// TODO: Allow querying IDs by name once it's available in the DB
			continue;
		}

		userIds.emplace_back(id);
	}

	ProcessUsers(userIds);
}

void Processor::ProcessUsers(const std::vector<s64>& userIds)
{
	UpdateBatch newUsers{_pDB, 10000};
	UpdateBatch newScores{_pDB, 10000};

	std::vector<User> users;
	for (s64 userId : userIds)
	{
		users.emplace_back(processSingleUser(
			0, // We want to update _all_ scores
			*_pDBSlave,
			newUsers,
			newScores,
			userId
		));
	}

	std::sort(std::begin(users), std::end(users), [](const User& a, const User& b) {
		if (a.GetPPRecord().Value != b.GetPPRecord().Value)
			return a.GetPPRecord().Value > b.GetPPRecord().Value;

		return a.Id() > b.Id();
	});

	Log(Info, "============================");
	Log(Info, "======= USER SUMMARY =======");
	Log(Info, "============================");
	Log(Info, "      User    Perf.     Acc.");
	Log(Info, "----------------------------");

	for (const auto& user : users)
		Log(Info, StrFormat("{0w10ar}  {1w5ar}pp  {2w6arp2}%", user.Id(), (s32)user.GetPPRecord().Value, user.GetPPRecord().Accuracy));

	Log(Info, "=============================");
}

std::shared_ptr<DatabaseConnection> Processor::newDBConnectionMaster()
{
	return std::make_shared<DatabaseConnection>(
		_config.MySQL_db_host,
		_config.MySQL_db_port,
		_config.MySQL_db_username,
		_config.MySQL_db_password,
		_config.MySQL_db_database
	);
}

std::shared_ptr<DatabaseConnection> Processor::newDBConnectionSlave()
{
	return std::make_shared<DatabaseConnection>(
		_config.MySQL_db_slave_host,
		_config.MySQL_db_slave_port,
		_config.MySQL_db_slave_username,
		_config.MySQL_db_slave_password,
		_config.MySQL_db_slave_database
	);
}

void Processor::queryAllBeatmapDifficulties()
{
	static const s32 step = 10000;

	Log(Info, "Retrieving all beatmap difficulties.");

	auto res = _pDBSlave->Query("SELECT MAX(`beatmap_id`),COUNT(DISTINCT `beatmap_id`) FROM `osu_beatmap_difficulty_attribs` WHERE 1");

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Could not find beatmap ID stats.");

	const s32 maxBeatmapId = res.S32(0);
	const s32 numBeatmaps = res.S32(1);
	for (s32 begin = 0; begin < maxBeatmapId; begin += step)
	{
		queryBeatmapDifficulty(begin, std::min(begin + step, maxBeatmapId+1));

		LogProgress(_beatmaps.size(), numBeatmaps);

		// This prevents stall checks to kill us during difficulty load
		_lastBeatmapSetPollTime = steady_clock::now();
	}

	Log(Success, StrFormat("Loaded difficulties for a total of {0} beatmaps.", _beatmaps.size()));
}

bool Processor::queryBeatmapDifficulty(s32 startId, s32 endId)
{
	static thread_local auto pDBSlave = newDBConnectionSlave();

	std::string query = StrFormat(
		"SELECT `osu_beatmaps`.`beatmap_id`,`countNormal`,`mods`,`attrib_id`,`value`,`approved`,`score_version` "
		"FROM `osu_beatmaps` "
		"JOIN `osu_beatmap_difficulty_attribs` ON `osu_beatmaps`.`beatmap_id` = `osu_beatmap_difficulty_attribs`.`beatmap_id` "
		"WHERE `osu_beatmap_difficulty_attribs`.`mode`={0} AND `approved` >= 1",
		_gamemode
	);

	if (endId == 0)
		query += StrFormat(" AND `osu_beatmaps`.`beatmap_id`={0}", startId);
	else
		query += StrFormat(" AND `osu_beatmaps`.`beatmap_id`>={0} AND `osu_beatmaps`.`beatmap_id`<{1}", startId, endId);

	auto res = pDBSlave->Query(query);

	bool success = res.NumRows() != 0;

	RWLock lock{&_beatmapMutex, success};

	while (res.NextRow())
	{
		s32 id = res.S32(0);

		if (_beatmaps.count(id) == 0)
			_beatmaps.emplace(std::make_pair(id, id));

		auto& beatmap = _beatmaps.at(id);

		beatmap.SetRankedStatus(static_cast<Beatmap::ERankedStatus>(res.S32(5)));
		beatmap.SetScoreVersion(static_cast<Beatmap::EScoreVersion>(res.S32(6)));
		beatmap.SetNumHitCircles(res.IsNull(1) ? 0 : res.S32(1));
		beatmap.SetDifficultyAttribute(
			static_cast<EMods>(res.U32(2)),
			_difficultyAttributes[res.S32(3)],
			res.F32(4)
		);
	}

	if (endId != 0) {
		return success;
	}

	if (_beatmaps.count(startId) == 0)
	{
		std::string message = StrFormat("Couldn't find beatmap /b/{0}.", startId);

		Log(Warning, message.c_str());
		_dataDog.Increment("osu.pp.difficulty.retrieval_not_found", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

		/*ProcessorException e{SRC_POS, message};
		m_CURL.SendToSentry(
			m_Config.SentryDomain,
			m_Config.SentryProjectID,
			m_Config.SentryPublicKey,
			m_Config.SentryPrivateKey,
			e,
			GamemodeName(m_Gamemode)
		);*/

		success = false;
	}
	else
	{
		Log(Success, StrFormat("Obtained beatmap difficulty of /b/{0}.", startId));
		_dataDog.Increment("osu.pp.difficulty.retrieval_success", 1, { StrFormat("mode:{0}", GamemodeTag(_gamemode)) });
	}

	return success;
}

void Processor::pollAndProcessNewScores()
{
	static const s64 s_lastScoreIdUpdateStep = 100;

	UpdateBatch newUsers{_pDB, 0};  // We want the updates to occur immediately
	UpdateBatch newScores{_pDB, 0}; // batches are used to conform the interface of processSingleUser

	// Obtain all new scores since the last poll and process them
	auto res = _pDBSlave->Query(StrFormat(
		"SELECT `score_id`,`user_id`,`pp` FROM `osu_scores{0}_high` WHERE `score_id` > {1} ORDER BY `score_id` ASC",
		GamemodeSuffix(_gamemode), _currentScoreId
	));

	// Only reset the poll timer when we find nothing. Otherwise we want to directly keep going
	if (res.NumRows() == 0)
		_lastScorePollTime = steady_clock::now();

	_dataDog.Gauge("osu.pp.score.amount_behind_newest", res.NumRows(), {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

	while (res.NextRow())
	{
		// Only process scores where pp is still null.
		if (!res.IsNull(2))
			continue;

		s64 ScoreId = res.S64(0);
		s64 UserId = res.S64(1);

		_currentScoreId = std::max(_currentScoreId, ScoreId);

		Log(Info, StrFormat("New score {0} in mode {1} by {2}.", ScoreId, GamemodeName(_gamemode), UserId));

		processSingleUser(
			ScoreId, // Only update the new score, old ones are caught by the background processor anyways
			*_pDB,
			newUsers,
			newScores,
			UserId
		);

		++_numScoresProcessedSinceLastStore;
		if (_numScoresProcessedSinceLastStore > s_lastScoreIdUpdateStep)
		{
			storeCount(*_pDB, lastScoreIdKey(), _currentScoreId);
			_numScoresProcessedSinceLastStore = 0;
		}

		_dataDog.Increment("osu.pp.score.processed_new", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});
		_dataDog.Gauge("osu.pp.db.pending_queries", _pDB->NumPendingQueries(), {
			StrFormat("mode:{0}", GamemodeTag(_gamemode)),
			"connection:main",
		});
	}
}

void Processor::pollAndProcessNewBeatmapSets()
{
	static thread_local auto pDBSlave = newDBConnectionSlave();

	_lastBeatmapSetPollTime = steady_clock::now();

	Log(Info, "Retrieving new beatmap sets.");

	auto res = pDBSlave->Query(StrFormat(
		"SELECT `beatmap_id`, `approved_date` "
		"FROM `osu_beatmapsets` JOIN `osu_beatmaps` ON `osu_beatmapsets`.`beatmapset_id` = `osu_beatmaps`.`beatmapset_id` "
		"WHERE `approved_date` > '{0}' "
		"ORDER BY `approved_date` ASC",
		_lastApprovedDate
	));

	Log(Success, StrFormat("Retrieved {0} new beatmaps.", res.NumRows()));

	while (res.NextRow())
	{
		_lastApprovedDate = res.String(1);
		queryBeatmapDifficulty(res.S32(0));

		_dataDog.Increment("osu.pp.difficulty.required_retrieval", 1, { StrFormat("mode:{0}", GamemodeTag(_gamemode)) });
	}
}

void Processor::queryBeatmapBlacklist()
{
	Log(Info, "Retrieving blacklisted beatmaps.");

	auto res = _pDBSlave->Query(StrFormat(
		"SELECT `beatmap_id` "
		"FROM `osu_beatmap_performance_blacklist` "
		"WHERE `mode`={0}", _gamemode
	));

	while (res.NextRow())
		_blacklistedBeatmapIds.insert(res.S32(0));

	Log(Success, StrFormat("Retrieved {0} blacklisted beatmaps.", _blacklistedBeatmapIds.size()));
}

void Processor::queryBeatmapDifficultyAttributes()
{
	Log(Info, "Retrieving difficulty attribute names.");

	u32 numEntries = 0;

	auto res = _pDBSlave->Query("SELECT `attrib_id`,`name` FROM `osu_difficulty_attribs` WHERE 1 ORDER BY `attrib_id` DESC");
	while (res.NextRow())
	{
		u32 id = res.S32(0);
		if (_difficultyAttributes.size() < id + 1)
			_difficultyAttributes.resize(id + 1);

		_difficultyAttributes[id] = Beatmap::DifficultyAttributeFromName(res.String(1));
		++numEntries;
	}

	Log(Success, StrFormat("Retrieved {0} difficulty attributes, stored in {1} entries.", numEntries, _difficultyAttributes.size()));
}

User Processor::processSingleUser(
	s64 selectedScoreId,
	DatabaseConnection& db,
	UpdateBatch& newUsers,
	UpdateBatch& newScores,
	s64 userId
)
{
	switch (_gamemode)
	{
	case EGamemode::Standard:
		return processSingleUserGeneric<StandardScore>(selectedScoreId, db, newUsers, newScores, userId);

	case EGamemode::Taiko:
		return processSingleUserGeneric<TaikoScore>(selectedScoreId, db, newUsers, newScores, userId);

	case EGamemode::CatchTheBeat:
		return processSingleUserGeneric<CatchTheBeatScore>(selectedScoreId, db, newUsers, newScores, userId);

	case EGamemode::Mania:
		return processSingleUserGeneric<ManiaScore>(selectedScoreId, db, newUsers, newScores, userId);

	default:
		throw ProcessorException(SRC_POS, StrFormat("Unknown gamemode requested. ({0})", _gamemode));
	}
}

template <class TScore>
User Processor::processSingleUserGeneric(
	s64 selectedScoreId,
	DatabaseConnection& db,
	UpdateBatch& newUsers,
	UpdateBatch& newScores,
	s64 userId
)
{
	static thread_local std::shared_ptr<DatabaseConnection> pDBSlave = newDBConnectionSlave();

	DatabaseConnection& dbSlave = *pDBSlave;

	static const f32 s_notableEventRatingThreshold = 1.0f / 21.5f;
	static const f32 s_notableEventRatingDifferenceMinimum = 5.0f;

	auto res = dbSlave.Query(StrFormat(
		"SELECT "
		"`score_id`,"
		"`user_id`,"
		"`beatmap_id`,"
		"`score`,"
		"`maxcombo`,"
		"`count300`,"
		"`count100`,"
		"`count50`,"
		"`countmiss`,"
		"`countgeki`,"
		"`countkatu`,"
		"`enabled_mods`,"
		"`pp` "
		"FROM `osu_scores{0}_high` "
		"WHERE `user_id`={1}", GamemodeSuffix(_gamemode), userId
	));

	User user{userId};
	std::vector<TScore> scoresThatNeedDBUpdate;

	{
		RWLock lock{&_beatmapMutex, false};

		// Process the data we got
		while (res.NextRow())
		{
			s64 scoreId = res.S64(0);
			s32 beatmapId = res.S32(2);

			EMods mods = static_cast<EMods>(res.S32(11));

			// Blacklisted maps don't count
			if (_blacklistedBeatmapIds.count(beatmapId) > 0)
				continue;

			// We don't want to look at scores on beatmaps we have no information about
			if (_beatmaps.count(beatmapId) == 0)
			{
				lock.Unlock();

				//Log(Warning, StrFormat("No difficulty information of beatmap {0} available. Ignoring for calculation.", BeatmapId));
				queryBeatmapDifficulty(beatmapId);

				lock.Lock();

				// If after querying we still didn't find anything, then we can just leave it.
				if (_beatmaps.count(beatmapId) == 0)
					continue;
			}

			s32 rankedStatus = _beatmaps.at(beatmapId).RankedStatus();
			if (rankedStatus < s_minRankedStatus || rankedStatus > s_maxRankedStatus)
				continue;

			TScore score = TScore{
				scoreId,
				_gamemode,
				res.S32(1), // user_id
				beatmapId,
				res.S32(3), // score
				res.S32(4), // maxcombo
				res.S32(5), // Num300
				res.S32(6), // Num100
				res.S32(7), // Num50
				res.S32(8), // NumMiss
				res.S32(9), // NumGeki
				res.S32(10), // NumKatu
				mods,
				_beatmaps.at(beatmapId),
			};

			user.AddScorePPRecord(score.CreatePPRecord());

			if (res.IsNull(12) || selectedScoreId == 0 || selectedScoreId == scoreId)
			{
				// Column 12 is the pp value of the score from the database.
				// Only update score if it differs a lot!
				if (res.IsNull(12) || fabs(res.F32(12) - score.TotalValue()) > 0.001f)
					scoresThatNeedDBUpdate.emplace_back(score);
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock{newScores.Mutex()};

		for (const auto& score : scoresThatNeedDBUpdate)
			score.AppendToUpdateBatch(newScores);
	}

	_dataDog.Increment("osu.pp.score.updated", scoresThatNeedDBUpdate.size(), {StrFormat("mode:{0}", GamemodeTag(_gamemode))}, 0.01f);

	user.ComputePPRecord();
	auto userPPRecord = user.GetPPRecord();

	// Check for notable event
	if (selectedScoreId > 0 && // We only check for notable events if a score has been selected
		!scoresThatNeedDBUpdate.empty() && // Did the score actually get found (this _should_ never be false, but better make sure)
		scoresThatNeedDBUpdate.front().TotalValue() > userPPRecord.Value * s_notableEventRatingThreshold)
	{
		_dataDog.Increment("osu.pp.score.notable_events", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

		const auto& score = scoresThatNeedDBUpdate.front();

		// Obtain user's previous pp rating for determining the difference
		auto res = dbSlave.Query(StrFormat(
			"SELECT `{0}` FROM `osu_user_stats{1}` WHERE `user_id`={2}",
			std::string{_config.UserPPColumnName},
			GamemodeSuffix(_gamemode),
			userId
		));

		while (res.NextRow())
		{
			if (res.IsNull(0))
				continue;

			f64 ratingChange = userPPRecord.Value - res.F32(0);

			// We don't want to log scores, that give less than a mere 5 pp
			if (ratingChange < s_notableEventRatingDifferenceMinimum)
				continue;

			Log(Info, StrFormat("Notable event: /b/{0} /u/{1}", score.BeatmapId(), userId));

			db.NonQueryBackground(StrFormat(
				"INSERT INTO "
				"osu_user_performance_change(user_id, mode, beatmap_id, performance_change, rank) "
				"VALUES({0},{1},{2},{3},null)",
				userId,
				_gamemode,
				score.BeatmapId(),
				ratingChange
			));
		}
	}

	newUsers.AppendAndCommit(StrFormat(
		"UPDATE `osu_user_stats{0}` "
		"SET `{1}`= CASE "
			// Set pp to 0 if the user is inactive or restricted.
			"WHEN CURDATE() > DATE_ADD(`last_played`, INTERVAL 3 MONTH) THEN 0 "
			"ELSE {2} "
		"END,"
		"`accuracy_new`={3} "
		"WHERE `user_id`={4} AND ABS(`{1}` - {2}) > 0.01;",
		GamemodeSuffix(_gamemode),
		std::string{_config.UserPPColumnName},
		userPPRecord.Value,
		userPPRecord.Accuracy,
		userId
	));

	_dataDog.Increment("osu.pp.user.amount_processed", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))}, 0.01f);

	return user;
}

void Processor::storeCount(DatabaseConnection& db, std::string key, s64 value)
{
	db.NonQueryBackground(StrFormat(
		"INSERT INTO `osu_counts`(`name`,`count`) VALUES('{0}',{1}) "
		"ON DUPLICATE KEY UPDATE `name`=VALUES(`name`),`count`=VALUES(`count`)",
		key, value
	));
}

s64 Processor::retrieveCount(DatabaseConnection& db, std::string key)
{
	auto res = db.Query(StrFormat(
		"SELECT `count` FROM `osu_counts` WHERE `name`='{0}'", key
	));

	while (res.NextRow())
		if (!res.IsNull(0))
			return res.S64(0);

	return -1;
}

PP_NAMESPACE_END
