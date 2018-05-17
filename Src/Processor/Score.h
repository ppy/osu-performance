#pragma once

#include "Beatmap.h"
#include "SharedEnums.h"

class UpdateBatch;

PP_NAMESPACE_BEGIN

class Score
{
public:
	struct PPRecord
	{
		s64 ScoreId;
		s32 BeatmapId;
		f32 Value;
		f32 Accuracy;
	};

	Score(
		s64 scoreId,
		EGamemode mode,
		s64 userId,
		s32 beatmapId,
		s32 score,
		s32 maxCombo,
		s32 num300,
		s32 num100,
		s32 num50,
		s32 numMiss,
		s32 numGeki,
		s32 numKatu,
		EMods mods
	);

	s64 Id() const { return _scoreId; }
	s64 UserId() const { return _userId; }
	s32 BeatmapId() const { return _beatmapId; }

	virtual f32 TotalValue() const = 0;
	virtual f32 Accuracy() const = 0;
	virtual s32 TotalHits() const = 0;
	virtual s32 TotalSuccessfulHits() const = 0;

	void AppendToUpdateBatch(UpdateBatch& batch) const;

	PPRecord CreatePPRecord() { return PPRecord{_scoreId, _beatmapId, TotalValue(), Accuracy()}; }

protected:
	s64 _scoreId;
	EGamemode _mode;
	s64 _userId;
	s32 _beatmapId;

	s32 _score;
	s32 _maxCombo;
	s32 _num300;
	s32 _num100;
	s32 _num50;
	s32 _numMiss;

	s32 _numGeki;
	s32 _numKatu;

	EMods _mods;
};

PP_NAMESPACE_END
