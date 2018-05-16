#include <PrecompiledHeader.h>

#include "Processor.h"

#include "Standard/StandardScore.h"
#include "CatchTheBeat/CatchTheBeatScore.h"
#include "Taiko/TaikoScore.h"
#include "Mania/ManiaScore.h"

#include "../Shared/Network/UpdateBatch.h"

#include <nlohmann/json.hpp>

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

const Beatmap::ERankedStatus Processor::s_minRankedStatus = Beatmap::Ranked;
const Beatmap::ERankedStatus Processor::s_maxRankedStatus = Beatmap::Approved;

Processor::Processor(EGamemode gamemode, const std::string& configFile)
: _gamemode{gamemode}
{
	Log(None,           "---------------------------------------------------");
	Log(None, StrFormat("---- pp processor for gamemode {0}", GamemodeName(gamemode)));
	Log(None,           "---------------------------------------------------");

	readConfig(configFile);

	_pDataDog = std::make_unique<DDog>(_config.DataDogHost, _config.DataDogPort);
	_pDataDog->Increment("osu.pp.startups", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

	_pDB = newDBConnectionMaster();
	_pDBSlave = newDBConnectionSlave();

	queryBeatmapBlacklist();
	queryBeatmapDifficultyAttributes();
	queryAllBeatmapDifficulties(16);
}

Processor::~Processor()
{
	Log(Info, "Shutting down.");
}

void Processor::MonitorNewScores()
{
	_lastScorePollTime = steady_clock::now();
	_lastBeatmapSetPollTime = steady_clock::now();

	Log(Info, "Monitoring new scores.");

	_currentScoreId = retrieveCount(*_pDB, lastScoreIdKey());

	auto res = _pDBSlave->Query("SELECT MAX(`approved_date`) FROM `osu_beatmapsets` WHERE 1");

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Couldn't find maximum approved date.");

	_lastApprovedDate = res.Get<std::string>(0);

	std::thread beatmapPollThread{[this]()
	{
		auto pDbSlave = newDBConnectionSlave();
		while (!_shallShutdown)
		{
			if (steady_clock::now() - _lastBeatmapSetPollTime > milliseconds{_config.DifficultyUpdateInterval})
				pollAndProcessNewBeatmapSets(*pDbSlave);
			else
				std::this_thread::sleep_for(milliseconds(100));
		}
	}};

	std::thread scorePollThread{[this]()
	{
		while (!_shallShutdown)
		{
			if (steady_clock::now() - _lastScorePollTime > milliseconds{_config.ScoreUpdateInterval})
				pollAndProcessNewScores();
			else
				std::this_thread::sleep_for(milliseconds(1));
		}
	}};

	scorePollThread.join();
	beatmapPollThread.join();
}

void Processor::ProcessAllUsers(bool reProcess, u32 numThreads)
{
	ThreadPool threadPool{numThreads};
	std::vector<std::shared_ptr<DatabaseConnection>> dbConnections;
	std::vector<std::shared_ptr<DatabaseConnection>> dbSlaveConnections;
	std::vector<UpdateBatch> newUsersBatches;
	std::vector<UpdateBatch> newScoresBatches;

	for (u32 i = 0; i < numThreads; ++i)
	{
		dbConnections.push_back(newDBConnectionMaster());
		dbSlaveConnections.push_back(newDBConnectionSlave());

		newUsersBatches.emplace_back(dbConnections[i], 10000);
		newScoresBatches.emplace_back(dbConnections[i], 10000);
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

	Log(Info, StrFormat("Processing all users starting with ID {0}.", begin));

	auto startTime = steady_clock::now();

	auto res = _pDBSlave->Query(StrFormat(
		"SELECT MAX(`user_id`),COUNT(`user_id`) FROM `osu_user_stats{0}` WHERE `user_id`>={1}",
		GamemodeSuffix(_gamemode), begin
	));

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Could not find user ID stats.");

	const s64 maxUserId = res.Get<s64>(0);
	const s64 numUsers = res.Get<s64>(1);

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
			s64 userId = res.Get<s64>(0);

			threadPool.EnqueueTask(
				[&, userId, currentConnection]()
				{
					processSingleUser(
						0, // We want to update _all_ scores
						*dbConnections[currentConnection],
						*dbSlaveConnections[currentConnection],
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

		LogProgress(numUsersProcessed, numUsers, steady_clock::now() - startTime);

		u32 numPendingQueries = 0;

		do
		{
			numPendingQueries = 0;
			for (auto& pDBConn : dbConnections)
				numPendingQueries += (u32)pDBConn->NumPendingQueries();

			_pDataDog->Gauge("osu.pp.db.pending_queries", numPendingQueries,
			{
				StrFormat("mode:{0}", GamemodeTag(_gamemode)),
				"connection:background",
			}, 0.01f);

			std::this_thread::sleep_for(milliseconds(10));
		}
		while (threadPool.GetNumTasksInSystem() > 0 || numPendingQueries > 0);

		// Update our user_id counter
		storeCount(*_pDB, lastUserIdKey(), begin);
	}

	Log(Success, StrFormat(
		"Processed all {0} users for {1}.",
		numUsers,
		durationToString(steady_clock::now() - startTime)
	));
}

void Processor::ProcessUsers(const std::vector<std::string>& userNames)
{
	std::vector<s64> userIds;
	for (const auto& name : userNames)
	{
		s64 id = xtoi64(name.c_str());
		if (id == 0)
		{
			// If the given string is not a number, try treating it as a username
			auto res = _pDBSlave->Query(StrFormat(
				"SELECT `user_id` FROM `{0}` WHERE `username`='{1}'",
				_config.UserMetadataTableName, name
			));

			if (!res.NextRow())
				continue;

			id = res.Get<s64>(0);
		}

		userIds.emplace_back(id);
	}

	ProcessUsers(userIds);
}

void Processor::ProcessUsers(const std::vector<s64>& userIds)
{
	UpdateBatch newUsers{_pDB, 10000};
	UpdateBatch newScores{_pDB, 10000};

	Log(Info, StrFormat("Processing {0} users.", userIds.size()));

	auto startTime = steady_clock::now();

	std::vector<User> users;
	for (s64 userId : userIds)
	{
		users.emplace_back(processSingleUser(
			0, // We want to update _all_ scores
			*_pDB,
			*_pDBSlave,
			newUsers,
			newScores,
			userId
		));

		LogProgress(users.size(), userIds.size(), steady_clock::now() - startTime);
	}

	Log(Info, StrFormat("Sorting {0} users.", users.size()));

	std::sort(std::begin(users), std::end(users), [](const User& a, const User& b) {
		if (a.GetPPRecord().Value != b.GetPPRecord().Value)
			return a.GetPPRecord().Value > b.GetPPRecord().Value;

		return a.Id() > b.Id();
	});

	Log(Success, StrFormat(
		"Processed {0} users for {1}.",
		users.size(),
		durationToString(steady_clock::now() - startTime)
	));

	Log(Info, "=============================================");
	Log(Info, "======= USER SUMMARY ========================");
	Log(Info, "=============================================");
	Log(Info, "            Name        Id    Perf.      Acc.");
	Log(Info, "---------------------------------------------");

	for (const auto& user : users)
	{
		// Try to obtain name
		std::string name = "<not-found>";
		auto res = _pDBSlave->Query(StrFormat(
			"SELECT `username` FROM `{0}` WHERE `user_id`='{1}'",
			_config.UserMetadataTableName, user.Id()
		));

		if (res.NextRow())
			name = res.Get<std::string>(0);

		Log(Info, StrFormat(
			"{0w16ar}  {1w8ar}  {2w5ar}pp  {3w6arp2} %",
			name,
			user.Id(),
			(s32)std::round(user.GetPPRecord().Value),
			user.GetPPRecord().Accuracy
		));
	}

	Log(Info, "=============================================");
}

void Processor::readConfig(const std::string& filename)
{
	using json = nlohmann::json;

	try
	{
		json j;

		{
			std::ifstream file{filename};
			file >> j;
		}

		_config.MySqlMasterHost =     j.value("mysql.master.host",     "localhost");
		_config.MySqlMasterPort =     j.value("mysql.master.port",     3306);
		_config.MySqlMasterUsername = j.value("mysql.master.username", "root");
		_config.MySqlMasterPassword = j.value("mysql.master.password", "");
		_config.MySqlMasterDatabase = j.value("mysql.master.database", "osu");

		// By default, use the same database as master and slave.
		_config.MySqlSlaveHost =     j.value("mysql.slave.host",     _config.MySqlMasterHost);
		_config.MySqlSlavePort =     j.value("mysql.slave.port",     _config.MySqlMasterPort);
		_config.MySqlSlaveUsername = j.value("mysql.slave.username", _config.MySqlMasterUsername);
		_config.MySqlSlavePassword = j.value("mysql.slave.password", _config.MySqlMasterPassword);
		_config.MySqlSlaveDatabase = j.value("mysql.slave.database", _config.MySqlMasterDatabase);

		_config.UserPPColumnName =      j.value("mysql.user-pp-column-name",      "rank_score");
		_config.UserMetadataTableName = j.value("mysql.user-metadata-table-name", "sample_users");

		_config.DifficultyUpdateInterval = j.value("poll.interval.difficulties", 10000);
		_config.ScoreUpdateInterval =      j.value("poll.interval.scores",       50);

		_config.SlackHookHost =     j.value("slack-hook.host",     "");
		_config.SlackHookKey =      j.value("slack-hook.key",      "");
		_config.SlackHookChannel =  j.value("slack-hook.channel",  "");
		_config.SlackHookUsername = j.value("slack-hook.username", "");
		_config.SlackHookIconURL =  j.value("slack-hook.icon-url", "");

		_config.SentryHost =       j.value("sentry.host",        "");
		_config.SentryProjectID =  j.value("sentry.project-id",  0);
		_config.SentryPublicKey =  j.value("sentry.public-key",  "");
		_config.SentryPrivateKey = j.value("sentry.private-key", "");

		_config.DataDogHost = j.value("data-dog.host", "127.0.0.1");
		_config.DataDogPort = j.value("data-dog.port", 8125);
	}
	catch (json::exception& e)
	{
		throw ProcessorException{SRC_POS, StrFormat("Failed to read config file '{0}': {1}", filename, e.what())};
	}
}

std::shared_ptr<DatabaseConnection> Processor::newDBConnectionMaster()
{
	return std::make_shared<DatabaseConnection>(
		_config.MySqlMasterHost,
		_config.MySqlMasterPort,
		_config.MySqlMasterUsername,
		_config.MySqlMasterPassword,
		_config.MySqlMasterDatabase
	);
}

std::shared_ptr<DatabaseConnection> Processor::newDBConnectionSlave()
{
	return std::make_shared<DatabaseConnection>(
		_config.MySqlSlaveHost,
		_config.MySqlSlavePort,
		_config.MySqlSlaveUsername,
		_config.MySqlSlavePassword,
		_config.MySqlSlaveDatabase
	);
}

void Processor::queryAllBeatmapDifficulties(u32 numThreads)
{
	static const s32 step = 10000;

	Log(Info, "Retrieving all beatmap difficulties.");

	auto startTime = steady_clock::now();

	auto res = _pDBSlave->Query(StrFormat(
		"SELECT MAX(`beatmap_id`),COUNT(*) FROM `osu_beatmaps` WHERE `approved` BETWEEN {0} AND {1} AND (`playmode`=0 OR `playmode`={2})",
		s_minRankedStatus, s_maxRankedStatus, _gamemode
	));

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Could not find beatmap ID stats.");

	const s32 maxBeatmapId = res.Get<s32>(0);
	const s32 numBeatmaps = res.Get<s32>(1);

	std::vector<std::shared_ptr<DatabaseConnection>> dbSlaveConnections;
	for (u32 i = 0; i < numThreads; ++i)
		dbSlaveConnections.emplace_back(newDBConnectionSlave());

	ThreadPool threadPool{numThreads};
	s32 threadIdx = 0;

	for (s32 begin = 0; begin < maxBeatmapId; begin += step)
	{
		auto& dbSlave = *dbSlaveConnections[threadIdx];
		threadIdx = (threadIdx + 1) % numThreads;

		threadPool.EnqueueTask([&, begin]() {
			queryBeatmapDifficulty(dbSlave, begin, std::min(begin + step, maxBeatmapId + 1));

			LogProgress(_beatmaps.size(), numBeatmaps, steady_clock::now() - startTime);
		});
	}

	while (threadPool.GetNumTasksInSystem() > 0)
	{
		std::this_thread::sleep_for(milliseconds{ 10 });
	}

	Log(Success, StrFormat(
		"Loaded difficulties for a total of {0} beatmaps for {1}.",
		_beatmaps.size(),
		durationToString(steady_clock::now() - startTime)
	));
}

bool Processor::queryBeatmapDifficulty(DatabaseConnection& dbSlave, s32 startId, s32 endId)
{
	std::string query = StrFormat(
		"SELECT `osu_beatmaps`.`beatmap_id`,`countNormal`,`mods`,`attrib_id`,`value`,`approved`,`score_version` "
		"FROM `osu_beatmaps` "
		"JOIN `osu_beatmap_difficulty_attribs` ON `osu_beatmaps`.`beatmap_id` = `osu_beatmap_difficulty_attribs`.`beatmap_id` "
		"WHERE (`osu_beatmaps`.`playmode`=0 OR `osu_beatmaps`.`playmode`={0}) AND `osu_beatmap_difficulty_attribs`.`mode`={0} AND `approved` BETWEEN {1} AND {2}",
		_gamemode, s_minRankedStatus, s_maxRankedStatus
	);

	if (endId == 0)
		query += StrFormat(" AND `osu_beatmaps`.`beatmap_id`={0}", startId);
	else
		query += StrFormat(" AND `osu_beatmaps`.`beatmap_id`>={0} AND `osu_beatmaps`.`beatmap_id`<{1}", startId, endId);

	auto res = dbSlave.Query(query);

	bool success = res.NumRows() != 0;

	RWLock lock{&_beatmapMutex, success};

	while (res.NextRow())
	{
		s32 id = res.Get<s32>(0);

		if (_beatmaps.count(id) == 0)
			_beatmaps.emplace(std::make_pair(id, id));

		auto& beatmap = _beatmaps.at(id);

		beatmap.SetRankedStatus(static_cast<Beatmap::ERankedStatus>(res.Get<std::underlying_type_t<Beatmap::ERankedStatus>>(5)));
		beatmap.SetScoreVersion(static_cast<Beatmap::EScoreVersion>(res.Get<std::underlying_type_t<Beatmap::EScoreVersion>>(6)));
		beatmap.SetNumHitCircles(res.IsNull(1) ? 0 : res.Get<s32>(1));
		beatmap.SetDifficultyAttribute(
			static_cast<EMods>(res.Get<std::underlying_type_t<EMods>>(2)),
			_difficultyAttributes[res.Get<s32>(3)],
			res.Get<f32>(4)
		);
	}

	if (endId != 0) {
		return success;
	}

	if (_beatmaps.count(startId) == 0)
	{
		std::string message = StrFormat("Couldn't find beatmap /b/{0}.", startId);

		Log(Warning, message.c_str());
		_pDataDog->Increment("osu.pp.difficulty.retrieval_not_found", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

		/*ProcessorException e{SRC_POS, message};
		m_CURL.SendToSentry(
			m_Config.SentryHost,
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
		_pDataDog->Increment("osu.pp.difficulty.retrieval_success", 1, { StrFormat("mode:{0}", GamemodeTag(_gamemode)) });
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

	_pDataDog->Gauge("osu.pp.score.amount_behind_newest", res.NumRows(), {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

	while (res.NextRow())
	{
		// Only process scores where pp is still null.
		if (!res.IsNull(2))
			continue;

		s64 ScoreId = res.Get<s64>(0);
		s64 UserId = res.Get<s64>(1);

		_currentScoreId = std::max(_currentScoreId, ScoreId);

		Log(Info, StrFormat("New score {0} in mode {1} by {2}.", ScoreId, GamemodeName(_gamemode), UserId));

		processSingleUser(
			ScoreId, // Only update the new score, old ones are caught by the background processor anyways
			*_pDB,
			*_pDBSlave,
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

		_pDataDog->Increment("osu.pp.score.processed_new", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});
		_pDataDog->Gauge("osu.pp.db.pending_queries", _pDB->NumPendingQueries(), {
			StrFormat("mode:{0}", GamemodeTag(_gamemode)),
			"connection:main",
		});
	}
}

void Processor::pollAndProcessNewBeatmapSets(DatabaseConnection& dbSlave)
{
	_lastBeatmapSetPollTime = steady_clock::now();

	Log(Info, "Retrieving new beatmap sets.");

	auto res = dbSlave.Query(StrFormat(
		"SELECT `beatmap_id`, `approved_date` "
		"FROM `osu_beatmapsets` JOIN `osu_beatmaps` ON `osu_beatmapsets`.`beatmapset_id` = `osu_beatmaps`.`beatmapset_id` "
		"WHERE `approved_date` > '{0}' "
		"ORDER BY `approved_date` ASC",
		_lastApprovedDate
	));

	Log(Success, StrFormat("Retrieved {0} new beatmaps.", res.NumRows()));

	while (res.NextRow())
	{
		_lastApprovedDate = res.Get<std::string>(1);
		queryBeatmapDifficulty(dbSlave, res.Get<s32>(0));

		_pDataDog->Increment("osu.pp.difficulty.required_retrieval", 1, { StrFormat("mode:{0}", GamemodeTag(_gamemode)) });
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
		_blacklistedBeatmapIds.insert(res.Get<s32>(0));

	Log(Success, StrFormat("Retrieved {0} blacklisted beatmaps.", _blacklistedBeatmapIds.size()));
}

void Processor::queryBeatmapDifficultyAttributes()
{
	Log(Info, "Retrieving difficulty attribute names.");

	u32 numEntries = 0;

	auto res = _pDBSlave->Query("SELECT `attrib_id`,`name` FROM `osu_difficulty_attribs` WHERE 1 ORDER BY `attrib_id` DESC");
	while (res.NextRow())
	{
		u32 id = res.Get<s32>(0);
		if (_difficultyAttributes.size() < id + 1)
			_difficultyAttributes.resize(id + 1);

		_difficultyAttributes[id] = Beatmap::DifficultyAttributeFromName(res.Get<std::string>(1));
		++numEntries;
	}

	Log(Success, StrFormat("Retrieved {0} difficulty attributes, stored in {1} entries.", numEntries, _difficultyAttributes.size()));
}

User Processor::processSingleUser(
	s64 selectedScoreId,
	DatabaseConnection& db,
	DatabaseConnection& dbSlave,
	UpdateBatch& newUsers,
	UpdateBatch& newScores,
	s64 userId
)
{
	switch (_gamemode)
	{
	case EGamemode::Standard:
		return processSingleUserGeneric<StandardScore>(selectedScoreId, db, dbSlave, newUsers, newScores, userId);

	case EGamemode::Taiko:
		return processSingleUserGeneric<TaikoScore>(selectedScoreId, db, dbSlave, newUsers, newScores, userId);

	case EGamemode::CatchTheBeat:
		return processSingleUserGeneric<CatchTheBeatScore>(selectedScoreId, db, dbSlave, newUsers, newScores, userId);

	case EGamemode::Mania:
		return processSingleUserGeneric<ManiaScore>(selectedScoreId, db, dbSlave, newUsers, newScores, userId);

	default:
		throw ProcessorException(SRC_POS, StrFormat("Unknown gamemode requested. ({0})", _gamemode));
	}
}

template <class TScore>
User Processor::processSingleUserGeneric(
	s64 selectedScoreId,
	DatabaseConnection& db,
	DatabaseConnection& dbSlave,
	UpdateBatch& newUsers,
	UpdateBatch& newScores,
	s64 userId
)
{
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
			s64 scoreId = res.Get<s64>(0);
			s32 beatmapId = res.Get<s32>(2);

			EMods mods = static_cast<EMods>(res.Get<std::underlying_type_t<EMods>>(11));

			// Blacklisted maps don't count
			if (_blacklistedBeatmapIds.count(beatmapId) > 0)
				continue;

			auto beatmapIt = _beatmaps.find(beatmapId);

			// We don't want to look at scores on beatmaps we have no information about
			if (beatmapIt == std::end(_beatmaps))
			{
				// If we couldn't find the beatmap of the _selected score_
				// we should probably re-check in the DB whether the beatmap recently appeared.
				// Specific scores are usually selected when new scores of players need to have
				// their pp computed, and those can in theory be on newly ranked maps.
				// While the pp processor queries newly ranked maps periodically, let's still
				// make absolutely sure here.
				if (selectedScoreId == scoreId)
				{
					lock.Unlock();
					queryBeatmapDifficulty(dbSlave, beatmapId);
					lock.Lock();
					beatmapIt = _beatmaps.find(beatmapId);

					// If after querying we still didn't find anything, then we can just leave it.
					if (beatmapIt == std::end(_beatmaps))
						continue;
				}

				continue;
			}

			const auto& beatmap = beatmapIt->second;

			s32 rankedStatus = beatmap.RankedStatus();
			if (rankedStatus < s_minRankedStatus || rankedStatus > s_maxRankedStatus)
				continue;

			TScore score = TScore{
				scoreId,
				_gamemode,
				res.Get<s32>(1), // user_id
				beatmapId,
				res.Get<s32>(3), // score
				res.Get<s32>(4), // maxcombo
				res.Get<s32>(5), // Num300
				res.Get<s32>(6), // Num100
				res.Get<s32>(7), // Num50
				res.Get<s32>(8), // NumMiss
				res.Get<s32>(9), // NumGeki
				res.Get<s32>(10), // NumKatu
				mods,
				beatmap,
			};

			user.AddScorePPRecord(score.CreatePPRecord());

			if (res.IsNull(12) || selectedScoreId == 0 || selectedScoreId == scoreId)
			{
				// Column 12 is the pp value of the score from the database.
				// Only update score if it differs a lot!
				if (res.IsNull(12) || fabs(res.Get<f32>(12) - score.TotalValue()) > 0.001f)
					scoresThatNeedDBUpdate.emplace_back(score);
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock{newScores.Mutex()};

		for (const auto& score : scoresThatNeedDBUpdate)
			score.AppendToUpdateBatch(newScores);
	}

	_pDataDog->Increment("osu.pp.score.updated", scoresThatNeedDBUpdate.size(), {StrFormat("mode:{0}", GamemodeTag(_gamemode))}, 0.01f);

	user.ComputePPRecord();
	auto userPPRecord = user.GetPPRecord();

	// Check for notable event
	if (selectedScoreId > 0 && // We only check for notable events if a score has been selected
		!scoresThatNeedDBUpdate.empty() && // Did the score actually get found (this _should_ never be false, but better make sure)
		scoresThatNeedDBUpdate.front().TotalValue() > userPPRecord.Value * s_notableEventRatingThreshold)
	{
		_pDataDog->Increment("osu.pp.score.notable_events", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

		const auto& score = scoresThatNeedDBUpdate.front();

		// Obtain user's previous pp rating for determining the difference
		auto res = dbSlave.Query(StrFormat(
			"SELECT `{0}` FROM `osu_user_stats{1}` WHERE `user_id`={2}",
			_config.UserPPColumnName,
			GamemodeSuffix(_gamemode),
			userId
		));

		while (res.NextRow())
		{
			if (res.IsNull(0))
				continue;

			f64 ratingChange = userPPRecord.Value - res.Get<f32>(0);

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
			"WHEN (CURDATE() > DATE_ADD(`last_played`, INTERVAL 3 MONTH) OR (SELECT `user_warnings` FROM `{5}` WHERE `user_id`={4}) > 0) THEN 0 "
			"ELSE {2} "
		"END,"
		"`accuracy_new`={3} "
		"WHERE `user_id`={4} AND ABS(`{1}` - {2}) > 0.01;",
		GamemodeSuffix(_gamemode),
		_config.UserPPColumnName,
		userPPRecord.Value,
		userPPRecord.Accuracy,
		userId,
		_config.UserMetadataTableName
	));

	_pDataDog->Increment("osu.pp.user.amount_processed", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))}, 0.01f);

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
			return res.Get<s64>(0);

	throw ProcessorException{SRC_POS, StrFormat("Unable to retrieve count '{0}'.", key)};
}

PP_NAMESPACE_END
