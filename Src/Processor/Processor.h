#pragma once

#include "Beatmap.h"
#include "CURL.h"
#include "DDog.h"
#include "User.h"
#include "SharedEnums.h"

#include "../Shared/Config.h"
#include "../Shared/Network/DatabaseConnection.h"

DEFINE_LOGGED_EXCEPTION(CProcessorException);

// Specify user id to test
//#define PLAYER_TESTING 124493

struct SScore;

class CProcessor
{
public:
	CProcessor(EGamemode gamemode, const std::string& configFile);
	~CProcessor();

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

private:
	static const std::array<const std::string, NumGamemodes> s_gamemodeSuffixes;
	static const std::array<const std::string, NumGamemodes> s_gamemodeNames;
	static const std::array<const std::string, NumGamemodes> s_gamemodeTags;

	static const CBeatmap::ERankedStatus s_minRankedStatus = CBeatmap::ERankedStatus::Ranked;
	static const CBeatmap::ERankedStatus s_maxRankedStatus = CBeatmap::ERankedStatus::Approved;

	std::string LastScoreIdKey()
	{
		return StrFormat("pp_last_score_id{0}", GamemodeSuffix(_gamemode));
	}

	std::string LastUserIdKey()
	{
		return StrFormat("pp_last_user_id{0}", GamemodeSuffix(_gamemode));
	}

	CConfig _config;

	std::shared_ptr<CDatabaseConnection> NewDBConnectionMaster();
	std::shared_ptr<CDatabaseConnection> NewDBConnectionSlave();

	// Difficulty data is held in RAM.
	// A few hundred megabytes.
	// Stored inside a hashmap with the beatmap ID as key
	std::unordered_map<s32, CBeatmap> _beatmaps;
	std::string _lastApprovedDate;

	void QueryBeatmapDifficulty();
	bool QueryBeatmapDifficulty(s32 startId, s32 endId = 0);

	std::shared_ptr<CDatabaseConnection> _pDB;
	std::shared_ptr<CDatabaseConnection> _pDBSlave;

	std::chrono::steady_clock::time_point _lastScorePollTime;
	std::chrono::steady_clock::time_point _lastBeatmapSetPollTime;

	s64 _currentScoreId;
	s64 _amountScoresProcessedSinceLastStore = 0;
	void PollAndProcessNewScores();
	void PollAndProcessNewBeatmapSets();

	std::unique_ptr<CScore> NewScore(
		s64 scoreId,
		EGamemode mode,
		s32 userId,
		s32 beatmapId,
		s32 score,
		s32 maxCombo,
		s32 amount300,
		s32 amount100,
		s32 amount50,
		s32 amountMiss,
		s32 amountGeki,
		s32 amountKatu,
		EMods mods
	);

	std::unordered_set<s32> _blacklistedBeatmapIds;
	void QueryBeatmapBlacklist();

	std::vector<CBeatmap::EDifficultyAttributeType> _difficultyAttributes;
	void QueryBeatmapDifficultyAttributes();

	// Not thread safe with beatmap data!
	void ProcessSingleUser(
		s64 selectedScoreId, // If this is not 0, then the score is looked at in isolation, triggering a notable event if it's good enough
		CDatabaseConnection& db,
		CUpdateBatch& newUsers,
		CUpdateBatch& newScores,
		s64 userId
	);

	void StoreCount(CDatabaseConnection& db, std::string key, s64 value);
	s64 RetrieveCount(CDatabaseConnection& db, std::string key);

	EGamemode _gamemode;

	CRWMutex _beatmapMutex;
	bool _shallShutdown = false;

	CCURL _curl;
	CDDog _dataDog;
};
