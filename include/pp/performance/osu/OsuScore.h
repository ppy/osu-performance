#pragma once

#include <pp/Common.h>
#include <pp/performance/Score.h>

PP_NAMESPACE_BEGIN

class OsuScore : public Score
{
public:

	OsuScore(
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
		EMods mods,
		const Beatmap& beatmap
	);

	f32 TotalValue() const override;
	f32 Accuracy() const override;
	s32 TotalHits() const override;
	s32 TotalSuccessfulHits() const override;

private:
	f32 _aimValue;
	f32 _speedValue;
	f32 _accValue;

	void computeTotalValue(const Beatmap& beatmap);
	f32 _totalValue;

	void computeAimValue(const Beatmap& beatmap);
	void computeSpeedValue(const Beatmap& beatmap);
	void computeAccValue(const Beatmap& beatmap);

	static constexpr f32 comboDistributionLocation = 930.58f;
	static constexpr f32 comboDistributionScale = 969.16f;
	static constexpr f32 comboTruncateMin = 0.073373031198542646f; // == std::exp(-std::exp(comboDistributionLocation / comboDistributionScale))
	f32 comboScaling(s32 scoreMaxCombo, s32 beatmapMaxCombo);
};

PP_NAMESPACE_END
