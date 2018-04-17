#pragma once

#include "Beatmap.h"
#include "CCURL.h"
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

	CProcessor(SharedEnums::EGamemode gamemode, bool reProcess);
	~CProcessor();


	static const std::string& GamemodeSuffix(SharedEnums::EGamemode gamemode)
	{
		return s_gamemodeSuffixes.at(gamemode);
	}

	static const std::string& GamemodeName(SharedEnums::EGamemode gamemode)
	{
		return s_gamemodeNames.at(gamemode);
	}

	static const std::string& GamemodeTag(SharedEnums::EGamemode gamemode)
	{
		return s_gamemodeTags.at(gamemode);
	}

private:
	static const std::array<const std::string, SharedEnums::AmountGamemodes> s_gamemodeSuffixes;
	static const std::array<const std::string, SharedEnums::AmountGamemodes> s_gamemodeNames;
	static const std::array<const std::string, SharedEnums::AmountGamemodes> s_gamemodeTags;

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
	static const std::string s_configFile;

	std::shared_ptr<CDatabaseConnection> NewDBConnectionMaster();
	std::shared_ptr<CDatabaseConnection> NewDBConnectionSlave();

	// Difficulty data is held in RAM.
	// A few hundred megabytes.
	// Stored inside a hashmap with the beatmap ID as key
	std::unordered_map<s32, CBeatmap> _beatmaps;
	std::string _lastApprovedDate;

	void Run(bool reProcess);

	void QueryBeatmapDifficulty();
	bool QueryBeatmapDifficulty(s32 startId, s32 endId = 0);

	void ProcessAllScores(bool reProcess);

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
		SharedEnums::EGamemode mode,
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
		SharedEnums::EMods mods
	);

	std::unordered_set<s32> _blacklistedBeatmapIds;
	void QueryBeatmapBlacklist();

	std::vector<CBeatmap::EDifficultyAttributeType> _difficultyAttributes;
	void QueryBeatmapDifficultyAttributes();

	// Not thread safe with beatmap data!
	void ProcessSingleUser(
		s64 selectedScoreId, // If this is not 0, then the score is looked at in isolation, triggering a notable event if it's good enough
		std::shared_ptr<CDatabaseConnection> pDB,
		std::shared_ptr<CUpdateBatch> newUsers,
		std::shared_ptr<CUpdateBatch> newScores,
		s64 userId
	);

	void StoreCount(CDatabaseConnection& db, std::string key, s64 value);
	s64 RetrieveCount(CDatabaseConnection& db, std::string key);

	SharedEnums::EGamemode _gamemode;

	CRWMutex _beatmapMutex;
	std::thread _backgroundScoreProcessingThread;
	std::thread _stallSupervisorThread;
	bool _shallShutdown = false;

	CCURL _curl;
	CDDog _dataDog;
};
