#pragma once

#include "Score.h"

class CTaikoScore : public CScore
{
public:
	CTaikoScore(
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
	void ComputeTotalValue();
	f32 _totalValue;

	void ComputeStrainValue(const CBeatmap& beatmap);
	void ComputeAccValue(const CBeatmap& beatmap);

	f32 _strainValue;
	f32 _accValue;
};
