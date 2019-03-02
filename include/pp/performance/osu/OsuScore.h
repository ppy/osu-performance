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
	f32 _dummyAimValue;
	f32 _dummySpeedValue;
	f32 _dummyAccValue;

	void computeTotalValue();
	f32 _totalValue;

	void computeAimValue(const Beatmap& beatmap);
	void computeSpeedValue(const Beatmap& beatmap);
	void computeAccValue(const Beatmap& beatmap);
	void dummyAimValue(const Beatmap& beatmap);
	void dummySpeedValue(const Beatmap& beatmap);
	void dummyAccValue(const Beatmap& beatmap);
};

PP_NAMESPACE_END
