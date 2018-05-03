#pragma once

#include "Score.h"

class TaikoScore : public Score
{
public:
	TaikoScore(
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
		const Beatmap& beatmap
	);

	f32 TotalValue() const override;
	f32 Accuracy() const override;
	s32 TotalHits() const override;
	s32 TotalSuccessfulHits() const override;

private:
	void computeTotalValue();
	f32 _totalValue;

	void computeStrainValue(const Beatmap& beatmap);
	void computeAccValue(const Beatmap& beatmap);

	f32 _strainValue;
	f32 _accValue;
};
