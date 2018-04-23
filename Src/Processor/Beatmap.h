#pragma once

#include "SharedEnums.h"

DEFINE_LOGGED_EXCEPTION(CBeatmapException);

class CInsertionBatch;

class CBeatmap
{
public:
	CBeatmap(s32 id);

	enum EDifficultyAttributeType : byte
	{
		Aim = 0,
		Speed,
		OD,
		AR,
		MaxCombo,
		Strain,
		HitWindow300,
		ScoreMultiplier,
	};

	enum ERankedStatus : s32
	{
		Ranked = 1,
		Approved = 2,
		Qualified = 3,
	};

	enum EScoreVersion : s32
	{
		ScoreV1 = 1,
		ScoreV2 = 2,
	};

	s32 Id() const { return _id; }

	ERankedStatus RankedStatus() const { return _rankedStatus; }
	EScoreVersion ScoreVersion() const { return _scoreVersion; }
	s32 AmountHitCircles() const { return _amountHitCircles; }
	f32 DifficultyAttribute(EMods mods, EDifficultyAttributeType type) const;

	void SetRankedStatus(ERankedStatus rankedStatus) { _rankedStatus = rankedStatus; }
	void SetScoreVersion(EScoreVersion scoreVersion) { _scoreVersion = scoreVersion; }
	void SetAmountHitCircles(s32 amountHitCircles) { _amountHitCircles = amountHitCircles; }
	void SetDifficultyAttribute(EMods mods, EDifficultyAttributeType type, f32 value);

	static EDifficultyAttributeType DifficultyAttributeFromName(const std::string& difficultyAttributeName)
	{
		return s_difficultyAttributes.at(difficultyAttributeName);
	}

private:
	static const EMods s_relevantDifficultyMods = static_cast<EMods>(
		DoubleTime | HalfTime | HardRock | Easy | keyMod
	);

	static EMods MaskRelevantDifficultyMods(EMods mods)
	{
		return static_cast<EMods>(mods & s_relevantDifficultyMods);
	}

	static const std::unordered_map<std::string, EDifficultyAttributeType> s_difficultyAttributes;

	// General information
	s32 _id;
	EGamemode _mode = EGamemode::Standard;

	// Calculated difficulty
	using difficulty_t = std::unordered_map<
		std::underlying_type_t<EMods>,
		std::unordered_map<
			std::underlying_type_t<EDifficultyAttributeType>,
			f32
		>
	>;

	difficulty_t _difficulty;

	// Additional info required for processor
	ERankedStatus _rankedStatus;
	EScoreVersion _scoreVersion;
	s32 _amountHitCircles = 0;
};
