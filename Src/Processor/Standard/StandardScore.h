#pragma once

#include "Score.h"

class CStandardScore : public CScore
{
public:

	CStandardScore(
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
		EMods mods,
		const CBeatmap& beatmap
	);

	f32 TotalValue() const override;
	f32 Accuracy() const override;
	s32 TotalHits() const override;
	s32 TotalSuccessfulHits() const override;

private:
	f32 _aimValue;
	f32 _speedValue;
	f32 _accValue;

	void computeTotalValue();
	f32 _totalValue;

	void computeAimValue(const CBeatmap& beatmap);
	void computeSpeedValue(const CBeatmap& beatmap);
	void computeAccValue(const CBeatmap& beatmap);
};
