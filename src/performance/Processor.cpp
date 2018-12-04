#include <pp/Common.h>
#include <pp/performance/Processor.h>

#include <pp/performance/osu/OsuScore.h>
#include <pp/performance/taiko/TaikoScore.h>
#include <pp/performance/catch/CatchScore.h>
#include <pp/performance/mania/ManiaScore.h>

#include <pp/shared/Threading.h>
#include <pp/shared/UpdateBatch.h>

#include <nlohmann/json.hpp>

using namespace std::chrono;

PP_NAMESPACE_BEGIN

const Beatmap::ERankedStatus Processor::s_minRankedStatus = Beatmap::Ranked;
const Beatmap::ERankedStatus Processor::s_maxRankedStatus = Beatmap::Approved;

Processor::Processor(EGamemode gamemode, const std::string& configFile)
: _gamemode{gamemode}
{
	tlog::none()
		<< "---------------------------------------------------\n"
		<< "---- pp processor for gamemode " << GamemodeName(gamemode) << '\n'
		<< "---------------------------------------------------";

	readConfig(configFile);

	_pDataDog = std::make_unique<DDog>(_config.DataDogHost, _config.DataDogPort);
	_pDataDog->Increment("osu.pp.startups", 1, {StrFormat("mode:{0}", GamemodeTag(_gamemode))});

	bool isDocker = std::getenv("DOCKER") != NULL;

	if (isDocker)
	{
		tlog::info() << "Waiting for database...";

		while (true)
		{
			try
			{
				_pDB = newDBConnectionMaster();

				if (retrieveCount(*_pDB, "docker_db_step") >= 2)
					break;
			}
			catch (const DatabaseException& dbe)
			{
				// Will occur if the database fails to connect (container not initialised)
			}
			catch (const ProcessorException& pe)
			{
				// Will occur if the retrieval of the docker step fails (tables not initialised)
			}

			std::this_thread::sleep_for(seconds(1));
		}
	}
	else
		_pDB = newDBConnectionMaster();

	_pDBSlave = newDBConnectionSlave();

	queryBeatmapBlacklist();
	queryBeatmapDifficultyAttributes();
	queryAllBeatmapDifficulties(16);

	if (isDocker)
		storeCount(*_pDB, "docker_db_step", 3);
}

Processor::~Processor()
{
	tlog::info() << "Shutting down.";
}

void Processor::MonitorNewScores()
{
	_lastScorePollTime = steady_clock::now();
	_lastBeatmapSetPollTime = steady_clock::now();

	tlog::info() << "Monitoring new scores.";

	_currentScoreId = retrieveCount(*_pDB, lastScoreIdKey());

	auto res = _pDBSlave->Query("SELECT MAX(`approved_date`) FROM `osu_beatmapsets` WHERE 1");

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Couldn't find maximum approved date.");

	_lastApprovedDate = (std::string)res[0];

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

	static const s32 s_maxNumUsers = 10000;

	s64 currentUserId; // Will be initialized in the next few lines
	if (reProcess)
	{
		currentUserId = 0;

		// Make sure in case of a restart we still do the full process, even if we didn't trigger a store before
		storeCount(*_pDB, lastUserIdKey(), currentUserId);
	}
	else
		currentUserId = retrieveCount(*_pDB, lastUserIdKey());

	auto res = _pDBSlave->Query(StrFormat(
		"SELECT COUNT(`user_id`) FROM `osu_user_stats{0}` WHERE `user_id`>={1}",
		GamemodeSuffix(_gamemode), currentUserId
	));

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Could not find user ID count.");

	const s64 numUsers = res[0];

	tlog::info() << StrFormat("Processing all users with ID larger than {0}.", currentUserId);
	auto progress = tlog::progress(numUsers);

	std::atomic<s64> numUsersProcessed{0};
	u32 currentConnection = 0;
	auto lastProgressUpdate = steady_clock::now();

	// We will break out as soon as there are no more results
	while (true)
	{
		res = _pDBSlave->Query(StrFormat(
			"SELECT "
			"`user_id`"
			"FROM `osu_user_stats{0}` "
			"WHERE `user_id`>{1} ORDER BY `user_id` ASC LIMIT {2}",
			GamemodeSuffix(_gamemode), currentUserId, s_maxNumUsers
		));

		if (res.NumRows() == 0)
			break;

		while (res.NextRow())
		{
			s64 userId = res[0];

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

					++numUsersProcessed;
				}
			);

			currentConnection = (currentConnection + 1) % numThreads;
			currentUserId = std::max(currentUserId, userId);

			// Shut down when requested!
			if (_shallShutdown)
				return;
		}

		u32 numPendingQueries = 0;

		do
		{
			u32 numPendingQueries = 0;
			for (auto& pDBConn : dbConnections)
				numPendingQueries += (u32)pDBConn->NumPendingQueries();

			_pDataDog->Gauge("osu.pp.db.pending_queries", numPendingQueries,
			{
				StrFormat("mode:{0}", GamemodeTag(_gamemode)),
				"connection:background",
			}, 0.01f);

			if (steady_clock::now() - lastProgressUpdate > milliseconds{100})
			{
				progress.update(numUsersProcessed);
				lastProgressUpdate += milliseconds{100};
			}

			std::this_thread::sleep_for(milliseconds{10});
		}
		while (threadPool.GetNumTasksInSystem() > 0 || numPendingQueries > 0);

		// Update our user_id counter
		storeCount(*_pDB, lastUserIdKey(), currentUserId);
	}

	tlog::success() << StrFormat(
		"Processed all {0} users for {1}.",
		numUsers,
		tlog::durationToString(progress.duration())
	);
}

void Processor::ProcessUsers(const std::vector<std::string>& userNames)
{
	std::vector<s64> userIds;
	for (const auto& name : userNames)
	{
		s64 id = strtoll(name.c_str(), 0, 0);
		if (id == 0)
		{
			// If the given string is not a number, try treating it as a username
			auto res = _pDBSlave->Query(StrFormat(
				"SELECT `user_id` FROM `{0}` WHERE `username`='{1}'",
				_config.UserMetadataTableName, name
			));

			if (!res.NextRow())
				continue;

			id = res[0];
		}

		userIds.emplace_back(id);
	}

	ProcessUsers(userIds);
}

void Processor::ProcessUsers(const std::vector<s64>& userIds)
{
	UpdateBatch newUsers{_pDB, 10000};
	UpdateBatch newScores{_pDB, 10000};

	tlog::info() << StrFormat("Processing {0} users.", userIds.size());
	auto progress = tlog::progress(userIds.size());

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

		progress.update(users.size());
	}

	tlog::info() << StrFormat("Sorting {0} users.", users.size());

	std::sort(std::begin(users), std::end(users), [](const User& a, const User& b) {
		if (a.GetPPRecord().Value != b.GetPPRecord().Value)
			return a.GetPPRecord().Value > b.GetPPRecord().Value;

		return a.Id() > b.Id();
	});

	tlog::success() << StrFormat(
		"Processed {0} users for {1}.",
		users.size(),
		tlog::durationToString(progress.duration())
	);

	tlog::info() << "=============================================";
	tlog::info() << "======= USER SUMMARY ========================";
	tlog::info() << "=============================================";
	tlog::info() << "            Name        Id    Perf.      Acc.";
	tlog::info() << "---------------------------------------------";

	for (const auto& user : users)
	{
		tlog::info() << StrFormat(
			"{0w16ar}  {1w8ar}  {2w5ar}pp  {3w6arp2} %",
			retrieveUserName(user.Id(), *_pDBSlave),
			user.Id(),
			(s32)std::round(user.GetPPRecord().Value),
			user.GetPPRecord().Accuracy
		);
	}

	tlog::info() << "=============================================";
}

void Processor::ProcessScores(const std::vector<s64>& scoreIds)
{
	UpdateBatch newUsers{_pDB, 10000};
	UpdateBatch newScores{_pDB, 10000};

	tlog::info() << StrFormat("Processing {0} scores.", scoreIds.size());
	auto progress = tlog::progress(scoreIds.size());

	struct Result {
		Score::PPRecord PP;
		s64 UserId;
		EMods Mods;
	};

	std::vector<Result> results;

	for (s64 scoreId : scoreIds)
	{
		// Get user ID for this particular score
		auto res = _pDBSlave->Query(StrFormat(
			"SELECT `user_id`,`enabled_mods` FROM `osu_scores{0}_high` WHERE `score_id`='{1}'",
			GamemodeSuffix(_gamemode), scoreId
		));

		if (!res.NextRow())
			continue;

		User user = processSingleUser(scoreId, *_pDB, *_pDBSlave, newUsers, newScores, res[0]);

		auto scoreIt = std::find_if(std::begin(user.Scores()), std::end(user.Scores()), [scoreId](const Score::PPRecord& a)
		{
			return a.ScoreId == scoreId;
		});

		if (scoreIt == std::end(user.Scores()))
		{
			tlog::warning() << StrFormat("Could not find score ID {0} in result set.", scoreId);
			continue;
		}

		results.push_back({*scoreIt, user.Id(), res[1]});
		progress.update(results.size());
	}

	tlog::info() << StrFormat("Sorting {0} results.", results.size());

	std::sort(std::begin(results), std::end(results), [](const Result& a, const Result& b) {
		if (a.PP.Value != b.PP.Value)
			return a.PP.Value > b.PP.Value;

		return a.PP.ScoreId > b.PP.ScoreId;
	});

	tlog::success() << StrFormat(
		"Processed {0} scores for {1}.",
		scoreIds.size(),
		tlog::durationToString(progress.duration())
	);

	tlog::info() << "================================================================================";
	tlog::info() << "======= SCORE SUMMARY ==========================================================";
	tlog::info() << "================================================================================";
	tlog::info() << "            Name     Perf.      Acc.  Beatmap - Mods";
	tlog::info() << "--------------------------------------------------------------------------------";

	for (const auto& result : results)
	{
		tlog::info() << StrFormat(
			"{0w16ar}  {1p1w6ar}pp  {2w6arp2} %  {3} - {4}",
			retrieveUserName(result.UserId, *_pDBSlave),
			result.PP.Value,
			result.PP.Accuracy * 100,
			retrieveBeatmapName(result.PP.BeatmapId, *_pDBSlave),
			ToString(result.Mods)
		);
	}

	tlog::info() << "================================================================================";
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

	auto res = _pDBSlave->Query(StrFormat(
		"SELECT MAX(`beatmap_id`),COUNT(*) FROM `osu_beatmaps` WHERE `approved` BETWEEN {0} AND {1} AND (`playmode`=0 OR `playmode`={2})",
		s_minRankedStatus, s_maxRankedStatus, _gamemode
	));

	if (!res.NextRow())
		throw ProcessorException(SRC_POS, "Could not find beatmap ID stats.");

	const s32 maxBeatmapId = res[0];
	const s32 numBeatmaps = res[1];

	tlog::info() << "Retrieving all beatmap difficulties.";
	auto progress = tlog::progress(numBeatmaps);

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

			progress.update(_beatmaps.size());
		});
	}

	while (threadPool.GetNumTasksInSystem() > 0)
	{
		std::this_thread::sleep_for(milliseconds{ 10 });
	}

	tlog::success() << StrFormat(
		"Loaded difficulties for a total of {0} beatmaps for {1}.",
		_beatmaps.size(),
		tlog::durationToString(progress.duration())
	);
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
		s32 id = res[0];

		if (_beatmaps.count(id) == 0)
			_beatmaps.emplace(std::make_pair(id, id));

		auto& beatmap = _beatmaps.at(id);

		beatmap.SetRankedStatus(res[5]);
		beatmap.SetScoreVersion(res[6]);
		beatmap.SetNumHitCircles(res.IsNull(1) ? 0 : (s32)res[1]);
		beatmap.SetDifficultyAttribute(res[2], _difficultyAttributes[(s32)res[3]], res[4]);
	}

	if (endId != 0) {
		return success;
	}

	if (_beatmaps.count(startId) == 0)
	{
		std::string message = StrFormat("Couldn't find beatmap /b/{0}.", startId);

		tlog::warning() << message.c_str();
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
		tlog::success() << StrFormat("Obtained beatmap difficulty of /b/{0}.", startId);
		_pDataDog->Increment("osu.pp.difficulty.retrieval_success", 1, { StrFormat("mode:{0}", GamemodeTag(_gamemode)) });
	}

	return success;
}

void Processor::pollAndProcessNewScores()
{
	static const s64 s_lastScoreIdUpdateStep = 100;
	static const s64 s_maxNumScores = 1000;

	UpdateBatch newUsers{_pDB, 0};  // We want the updates to occur immediately
	UpdateBatch newScores{_pDB, 0}; // batches are used to conform the interface of processSingleUser

	// Obtain all new scores since the last poll and process them
	auto res = _pDBSlave->Query(StrFormat(
		"SELECT `score_id`,`user_id`,`pp` "
		"FROM `osu_scores{0}_high` "
		"WHERE `score_id` > {1} ORDER BY `score_id` ASC LIMIT {3}",
		GamemodeSuffix(_gamemode), _currentScoreId, _config.UserMetadataTableName, s_maxNumScores
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


		s64 scoreId = res[0];
		s64 userId = res[1];

		_currentScoreId = std::max(_currentScoreId, scoreId);

		User user = processSingleUser(
			scoreId, // Only update the new score, old ones are caught by the background processor anyways
			*_pDB,
			*_pDBSlave,
			newUsers,
			newScores,
			userId
		);

		auto scoreIt = std::find_if(std::begin(user.Scores()), std::end(user.Scores()), [scoreId](const Score::PPRecord& a)
		{
			return a.ScoreId == scoreId;
		});

		if (scoreIt == std::end(user.Scores()))
		{
			tlog::warning() << StrFormat("Could not find score ID {0} in result set.", scoreId);
			continue;
		}

		tlog::info() << StrFormat(
			"Score={0w10ar} {1p1w6ar}pp {2p2w6ar}% | User={3w8ar} {4p1w7ar}pp {5p2w6ar}% | Beatmap={6w7ar}",
			scoreId, scoreIt->Value, scoreIt->Accuracy * 100,
			userId, user.GetPPRecord().Value, user.GetPPRecord().Accuracy,
			scoreIt->BeatmapId
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

	tlog::info() << "Retrieving new beatmap sets.";

	auto res = dbSlave.Query(StrFormat(
		"SELECT `beatmap_id`, `approved_date` "
		"FROM `osu_beatmapsets` JOIN `osu_beatmaps` ON `osu_beatmapsets`.`beatmapset_id` = `osu_beatmaps`.`beatmapset_id` "
		"WHERE `approved_date` > '{0}' "
		"ORDER BY `approved_date` ASC",
		_lastApprovedDate
	));

	tlog::success() << StrFormat("Retrieved {0} new beatmaps.", res.NumRows());

	while (res.NextRow())
	{
		_lastApprovedDate = (std::string)res[1];
		queryBeatmapDifficulty(dbSlave, res[0]);

		_pDataDog->Increment("osu.pp.difficulty.required_retrieval", 1, { StrFormat("mode:{0}", GamemodeTag(_gamemode)) });
	}
}

void Processor::queryBeatmapBlacklist()
{
	tlog::info() << "Retrieving blacklisted beatmaps.";

	auto res = _pDBSlave->Query(StrFormat(
		"SELECT `beatmap_id` "
		"FROM `osu_beatmap_performance_blacklist` "
		"WHERE `mode`={0}", _gamemode
	));

	while (res.NextRow())
		_blacklistedBeatmapIds.insert(res[0]);

	tlog::success() << StrFormat("Retrieved {0} blacklisted beatmaps.", _blacklistedBeatmapIds.size());
}

void Processor::queryBeatmapDifficultyAttributes()
{
	tlog::info() << "Retrieving difficulty attribute names.";

	u32 numEntries = 0;

	auto res = _pDBSlave->Query("SELECT `attrib_id`,`name` FROM `osu_difficulty_attribs` WHERE 1 ORDER BY `attrib_id` DESC");
	while (res.NextRow())
	{
		u32 id = res[0];
		if (_difficultyAttributes.size() < id + 1)
			_difficultyAttributes.resize(id + 1);

		_difficultyAttributes[id] = Beatmap::DifficultyAttributeFromName(res[1]);
		++numEntries;
	}

	tlog::success() << StrFormat("Retrieved {0} difficulty attributes, stored in {1} entries.", numEntries, _difficultyAttributes.size());
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
	case EGamemode::Osu:
		return processSingleUserGeneric<OsuScore>(selectedScoreId, db, dbSlave, newUsers, newScores, userId);

	case EGamemode::Taiko:
		return processSingleUserGeneric<TaikoScore>(selectedScoreId, db, dbSlave, newUsers, newScores, userId);

	case EGamemode::Catch:
		return processSingleUserGeneric<CatchScore>(selectedScoreId, db, dbSlave, newUsers, newScores, userId);

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
			s64 scoreId = res[0];
			s32 beatmapId = res[2];

			EMods mods = res[11];

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
				res[1], // user_id
				beatmapId,
				res[3], // score
				res[4], // maxcombo
				res[5], // Num300
				res[6], // Num100
				res[7], // Num50
				res[8], // NumMiss
				res[9], // NumGeki
				res[10], // NumKatu
				mods,
				beatmap,
			};

			user.AddScorePPRecord(score.CreatePPRecord());

			// Column 12 is the pp value of the score from the database.
			// Only update score if it differs a lot!
			if (res.IsNull(12) || fabs((f32)res[12] - score.TotalValue()) > 0.001f)
			{
				// Ensure the selected score is in the front if it exists
				if (selectedScoreId == scoreId)
					scoresThatNeedDBUpdate.insert(std::begin(scoresThatNeedDBUpdate), score);
				else
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
	if (!scoresThatNeedDBUpdate.empty() && scoresThatNeedDBUpdate.front().Id() == selectedScoreId && // Did the score actually get found (this _should_ never be false, but better make sure)
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

			f64 ratingChange = userPPRecord.Value - (f64)res[0];

			// We don't want to log scores, that give less than a mere 5 pp
			if (ratingChange < s_notableEventRatingDifferenceMinimum)
				continue;

			tlog::info() << StrFormat("Notable event: s{0} u{1} b{2}", score.Id(), userId, score.BeatmapId());

			db.NonQueryBackground(StrFormat(
				"INSERT INTO "
				"osu_user_performance_change(user_id, mode, beatmap_id, performance_change, `rank`) "
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
			return res[0];

	throw ProcessorException{SRC_POS, StrFormat("Unable to retrieve count '{0}'.", key)};
}

std::string Processor::retrieveUserName(s64 userId, DatabaseConnection& db) const
{
	auto res = db.Query(StrFormat(
		"SELECT `username` FROM `{0}` WHERE `user_id`='{1}'",
		_config.UserMetadataTableName, userId
	));

	if (!res.NextRow() || res.IsNull(0))
		return "<not-found>";

	return res[0];
}

std::string Processor::retrieveBeatmapName(s32 beatmapId, DatabaseConnection& db) const
{
	auto res = db.Query(StrFormat("SELECT `filename` FROM `osu_beatmaps` WHERE `beatmap_id`='{0}'", beatmapId));

	if (!res.NextRow() || res.IsNull(0))
		return "<not-found>";

	std::string result = res[0];

	// Strip trailing ".osu"
	if (result.size() > 4)
		result = result.substr(0, result.size()-4);

	return result;
}

PP_NAMESPACE_END
