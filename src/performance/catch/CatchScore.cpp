#include <pp/Common.h>
#include <pp/performance/catch/CatchScore.h>

PP_NAMESPACE_BEGIN

CatchScore::CatchScore(
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
) : Score{scoreId, mode, userId, beatmapId, score, maxCombo, num300, num100, num50, numMiss, numGeki, numKatu, mods}
{
	// Don't count scores made with supposedly unranked mods
	if ((_mods & EMods::Relax) > 0 ||
		(_mods & EMods::Relax2) > 0 ||
		(_mods & EMods::Autoplay) > 0)
	{
		_value = 0;
		return;
	}

	// We are heavily relying on aim in catch the beat
	_value = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, Beatmap::Aim) / 0.0049f) - 4.0f, 2.0f) / 100000.0f;

	// Longer maps are worth more. "Longer" means how many hits there are which can contribute to combo
	int numTotalHits = totalComboHits();

	// Longer maps with more movements are worth more
	f32 lengthBonusFactor = static_cast<f32>(numTotalHits) * 0.75f + beatmap.DifficultyAttribute(_mods, Beatmap::DirectionChangeCount) / 1.8f;

	f32 lengthBonus = log10(lengthBonusFactor + 100.0f) * 0.7f - 1.3f;

	// Longer maps are worth more
	_value *= lengthBonus;

	// Penalize misses exponentially. This mainly fixes tag4 maps and the likes until a per-hitobject solution is available
	_value *= pow(0.97f, _numMiss);

	// Combo scaling
	float beatmapMaxCombo = beatmap.DifficultyAttribute(_mods, Beatmap::MaxCombo);
	if (beatmapMaxCombo > 0)
		_value *= std::min<f32>(pow(static_cast<f32>(_maxCombo), 0.8f) / pow(beatmapMaxCombo, 0.8f), 1.0f);

	f32 approachRate = beatmap.DifficultyAttribute(_mods, Beatmap::AR);
	f32 approachRateFactor = 1.0f;
	if (approachRate > 9.0f)
		approachRateFactor += 0.1f * (approachRate - 9.0f); // 10% for each AR above 9
	if (approachRate > 10.0f)
		approachRateFactor += 0.1f * (approachRate - 10.0f); // Additional 10% at AR 11, 30% total
	else if (approachRate < 8.0f)
		approachRateFactor += 0.04f * (8.0f - approachRate); // 4% for each AR below 8

	_value *= approachRateFactor;

	if ((_mods & EMods::Hidden) > 0)
	{
		_value *= 1.05f + 0.17f * (10.0f - std::min(10.0f, approachRate)); // 20% for each AR below 10
	}

	if ((_mods & EMods::Flashlight) > 0)
	{
		// Apply length bonus again if flashlight is on simply because it becomes a lot harder on longer maps.
		_value *= 1.35f * lengthBonus;

		if (approachRate > 8.0f)
			_value *= 0.1f * (approachRate - 8.0f); // 10% for each AR above 8
		if (approachRate < 8.0f)
			_value *= 0.07f * (approachRate - 8.0f); // -7% for each AR below 8
	}

	// Scale the aim value with accuracy _slightly_
	_value *= pow(Accuracy(), 7.5f);

	// Custom multiplier for HalfTime -> slower catcher = easier to control
	if ((_mods & EMods::HalfTime) > 0)
		_value *= 0.85f;

	// Custom multiplier for DoubleTime -> faster catcher = harder to control
	if ((_mods & EMods::DoubleTime) > 0)
		_value *= 1.10f;

	// Custom multipliers for NoFail and SpunOut.
	if ((_mods & EMods::NoFail) > 0)
		_value *= 0.90f;

	if ((_mods & EMods::SpunOut) > 0)
		_value *= 0.95f;
}

f32 CatchScore::TotalValue() const
{
	return _value;
}

f32 CatchScore::Accuracy() const
{
	if (TotalHits() == 0)
		return 0;

	return Clamp(static_cast<f32>(TotalSuccessfulHits()) / TotalHits(), 0.0f, 1.0f);
}

s32 CatchScore::TotalHits() const
{
	return _num50 + _num100 + _num300 + _numMiss + _numKatu;
}

s32 CatchScore::TotalSuccessfulHits() const
{
	return _num50 + _num100 + _num300;
}

s32 CatchScore::totalComboHits() const
{
	return _num300 + _num100 + _numMiss;
}

PP_NAMESPACE_END
