#pragma once

#include "Beatmap.h"
#include "SharedEnums.h"

class CDatabaseConnection;
class CInsertionBatch;
class CUpdateBatch;

class CScore
{
public:
	struct SPPRecord
	{
		s64 ScoreId;
		s32 BeatmapId;
		f32 Value;
		f32 Accuracy;
	};

	CScore(
		s64 scoreId,
		EGamemode mode,
		s32 userId,
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
	~CScore() = default;

	s32 UserId() const { return _userId; }
	s32 BeatmapId() const { return _beatmapId; }

	virtual f32 TotalValue() const = 0;
	virtual f32 Accuracy() const = 0;
	virtual s32 TotalHits() const = 0;
	virtual s32 TotalSuccessfulHits() const = 0;

	void AppendToUpdateBatch(CUpdateBatch& batch);

	SPPRecord PPRecord() { return SPPRecord{_scoreId, _beatmapId, TotalValue(), Accuracy()}; }

protected:
	s64 _scoreId;
	EGamemode _mode;
	s32 _userId;
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
