#pragma once

#include <pp/Common.h>
#include <pp/performance/Score.h>

PP_NAMESPACE_BEGIN

class CatchScore : public Score
{
public:
	CatchScore(
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
	f32 _value;
	s32 totalComboHits() const;
};

PP_NAMESPACE_END
