#pragma once

#include "Score.h"

class CStandardScore : public CScore
{
public:

	CStandardScore(
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
		SharedEnums::EMods mods,
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

	void ComputeTotalValue();
	f32 _totalValue;

	void ComputeAimValue(const CBeatmap& beatmap);
	void ComputeSpeedValue(const CBeatmap& beatmap);
	void ComputeAccValue(const CBeatmap& beatmap);
};
