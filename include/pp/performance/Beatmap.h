#pragma once

#include <pp/Common.h>

#include <unordered_map>

PP_NAMESPACE_BEGIN

DEFINE_EXCEPTION(BeatmapException);

class Beatmap
{
public:
	Beatmap(s32 id);

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
		DirectionChangeCount,

		NumTypes,
	};

	enum ERankedStatus : s32
	{
		Ranked = 1,
		Approved = 2,
		Qualified = 3,
		Loved = 4,
	};

	enum EScoreVersion : s32
	{
		ScoreV1 = 1,
		ScoreV2 = 2,
	};

	s32 Id() const { return _id; }

	ERankedStatus RankedStatus() const { return _rankedStatus; }
	EScoreVersion ScoreVersion() const { return _scoreVersion; }
	s32 NumHitCircles() const { return _numHitCircles; }
	f32 DifficultyAttribute(EMods mods, EDifficultyAttributeType type) const;

	void SetRankedStatus(ERankedStatus rankedStatus) { _rankedStatus = rankedStatus; }
	void SetScoreVersion(EScoreVersion scoreVersion) { _scoreVersion = scoreVersion; }
	void SetNumHitCircles(s32 numHitCircles) { _numHitCircles = numHitCircles; }
	void SetDifficultyAttribute(EMods mods, EDifficultyAttributeType type, f32 value);

	static EDifficultyAttributeType DifficultyAttributeFromName(const std::string& difficultyAttributeName)
	{
		return s_difficultyAttributes.at(difficultyAttributeName);
	}

private:
	static const std::unordered_map<std::string, EDifficultyAttributeType> s_difficultyAttributes;

	// General information
	s32 _id;
	EGamemode _mode = EGamemode::Osu;

	// Calculated difficulty
	using difficulty_t = std::unordered_map<
		std::underlying_type_t<EMods>,
		std::array<f32, NumTypes>
	>;

	difficulty_t _difficulty;

	// Additional info required for processor
	ERankedStatus _rankedStatus;
	EScoreVersion _scoreVersion;
	s32 _numHitCircles = 0;
};

PP_NAMESPACE_END
