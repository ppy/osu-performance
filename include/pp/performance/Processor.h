#pragma once

#include <pp/Common.h>

#include <pp/performance/Beatmap.h>
#include <pp/performance/CURL.h>
#include <pp/performance/DDog.h>
#include <pp/performance/User.h>

#include <pp/shared/DatabaseConnection.h>
#include <pp/shared/Threading.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

PP_NAMESPACE_BEGIN

DEFINE_EXCEPTION(ProcessorException);

class Processor
{
public:
	Processor(EGamemode gamemode, const std::string& configFile);
	~Processor();

	void MonitorNewScores();
	void ProcessAllUsers(bool reProcess, u32 numThreads);
	void ProcessUsers(const std::vector<std::string>& userNames);
	void ProcessUsers(const std::vector<s64>& userIds);
	void ProcessScores(const std::vector<s64>& scoreIds);

private:
	static const Beatmap::ERankedStatus s_minRankedStatus;
	static const Beatmap::ERankedStatus s_maxRankedStatus;

	std::string lastScoreIdKey()
	{
		return StrFormat("pp_last_score_id{0}", GamemodeSuffix(_gamemode));
	}

	std::string lastUserIdKey()
	{
		return StrFormat("pp_last_user_id{0}", GamemodeSuffix(_gamemode));
	}

	struct
	{
		std::string MySqlMasterHost;
		s16 MySqlMasterPort;
		std::string MySqlMasterUsername;
		std::string MySqlMasterPassword;
		std::string MySqlMasterDatabase;

		// By default, use the same database as master and slave.
		std::string MySqlSlaveHost;
		s16 MySqlSlavePort;
		std::string MySqlSlaveUsername;
		std::string MySqlSlavePassword;
		std::string MySqlSlaveDatabase;

		s32 DifficultyUpdateInterval;
		s32 ScoreUpdateInterval;

		std::string UserPPColumnName;
		std::string UserMetadataTableName;

		bool WriteAllPPChanges;

		std::string SlackHookHost;
		std::string SlackHookKey;
		std::string SlackHookChannel;
		std::string SlackHookUsername;
		std::string SlackHookIconURL;

		std::string SentryHost;
		s32 SentryProjectID;
		std::string SentryPublicKey;
		std::string SentryPrivateKey;

		std::string DataDogHost;
		s16 DataDogPort;
	} _config;

	void readConfig(const std::string& filename);

	std::shared_ptr<DatabaseConnection> newDBConnectionMaster();
	std::shared_ptr<DatabaseConnection> newDBConnectionSlave();

	// Difficulty data is held in RAM.
	// A few hundred megabytes.
	// Stored inside a hashmap with the beatmap ID as key
	std::unordered_map<s32, Beatmap> _beatmaps;
	std::string _lastApprovedDate;

	void queryAllBeatmapDifficulties(u32 numThreads);
	bool queryBeatmapDifficulty(DatabaseConnection& dbSlave, s32 startId, s32 endId = 0);

	std::shared_ptr<DatabaseConnection> _pDB;
	std::shared_ptr<DatabaseConnection> _pDBSlave;

	std::chrono::steady_clock::time_point _lastScorePollTime;
	std::chrono::steady_clock::time_point _lastBeatmapSetPollTime;

	s64 _currentScoreId;
	s64 _currentQueueId;
	s64 _numScoresProcessedSinceLastStore = 0;
	void pollAndProcessNewScores();
	void pollAndProcessNewBeatmapSets(DatabaseConnection& dbSlave);

	std::unordered_set<s32> _blacklistedBeatmapIds;
	void queryBeatmapBlacklist();

	std::vector<Beatmap::EDifficultyAttributeType> _difficultyAttributes;
	void queryBeatmapDifficultyAttributes();

	// Not thread safe with beatmap data!
	User processSingleUser(
		s64 selectedScoreId, // If this is not 0, then the score is looked at in isolation, triggering a notable event if it's good enough
		DatabaseConnection& db,
		DatabaseConnection& dbSlave,
		UpdateBatch& newUsers,
		UpdateBatch& newScores,
		s64 userId
	);

	template <class TScore>
	User processSingleUserGeneric(
		s64 selectedScoreId, // If this is not 0, then the score is looked at in isolation, triggering a notable event if it's good enough
		DatabaseConnection& db,
		DatabaseConnection& dbSlave,
		UpdateBatch& newUsers,
		UpdateBatch& newScores,
		s64 userId
	);

	void storeCount(DatabaseConnection& db, std::string key, s64 value);
	s64 retrieveCount(DatabaseConnection& db, std::string key);

	std::string retrieveUserName(s64 userId, DatabaseConnection& db) const;
	std::string retrieveBeatmapName(s32 beatmapId, DatabaseConnection& db) const;

	EGamemode _gamemode;

	RWMutex _beatmapMutex;
	bool _shallShutdown = false;

	CURL _curl;
	std::unique_ptr<DDog> _pDataDog;
};

PP_NAMESPACE_END
