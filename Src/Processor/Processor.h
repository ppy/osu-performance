#pragma once

#include "Beatmap.h"
#include "CURL.h"
#include "DDog.h"
#include "User.h"
#include "SharedEnums.h"

#include "../Shared/Config.h"
#include "../Shared/Network/DatabaseConnection.h"

PP_NAMESPACE_BEGIN

DEFINE_LOGGED_EXCEPTION(ProcessorException);

class Processor
{
public:
	Processor(EGamemode gamemode, const std::string& configFile);
	~Processor();

	static const std::string& GamemodeSuffix(EGamemode gamemode)
	{
		return s_gamemodeSuffixes.at(gamemode);
	}

	static const std::string& GamemodeName(EGamemode gamemode)
	{
		return s_gamemodeNames.at(gamemode);
	}

	static const std::string& GamemodeTag(EGamemode gamemode)
	{
		return s_gamemodeTags.at(gamemode);
	}

	void MonitorNewScores();
	void ProcessAllUsers(bool reProcess, u32 numThreads);
	void ProcessUsers(const std::vector<std::string>& userNames);
	void ProcessUsers(const std::vector<s64>& userIds);

private:
	static const std::array<const std::string, NumGamemodes> s_gamemodeSuffixes;
	static const std::array<const std::string, NumGamemodes> s_gamemodeNames;
	static const std::array<const std::string, NumGamemodes> s_gamemodeTags;

	static const Beatmap::ERankedStatus s_minRankedStatus = Beatmap::ERankedStatus::Ranked;
	static const Beatmap::ERankedStatus s_maxRankedStatus = Beatmap::ERankedStatus::Approved;

	std::string lastScoreIdKey()
	{
		return StrFormat("pp_last_score_id{0}", GamemodeSuffix(_gamemode));
	}

	std::string lastUserIdKey()
	{
		return StrFormat("pp_last_user_id{0}", GamemodeSuffix(_gamemode));
	}

	Config _config;

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

	EGamemode _gamemode;

	RWMutex _beatmapMutex;
	bool _shallShutdown = false;

	CURL _curl;
	DDog _dataDog;
};

PP_NAMESPACE_END
