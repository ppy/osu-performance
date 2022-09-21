#include <pp/Common.h>
#include <pp/performance/osu/OsuScore.h>

PP_NAMESPACE_BEGIN

OsuScore::OsuScore(
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
	const Beatmap &beatmap) : Score{scoreId, mode, userId, beatmapId, score, maxCombo, num300, num100, num50, numMiss, numGeki, numKatu, mods}
{
	computeEffectiveMissCount(beatmap);

	computeAimValue(beatmap);
	computeSpeedValue(beatmap);
	computeAccuracyValue(beatmap);
	computeFlashlightValue(beatmap);

	computeTotalValue(beatmap);
}

f32 OsuScore::TotalValue() const
{
	return _totalValue;
}

f32 OsuScore::Accuracy() const
{
	if (TotalHits() == 0)
		return 0;

	return Clamp(
		static_cast<f32>(_num50 * 50 + _num100 * 100 + _num300 * 300) / (TotalHits() * 300), 0.0f, 1.0f);
}

s32 OsuScore::TotalHits() const
{
	return _num50 + _num100 + _num300 + _numMiss;
}

s32 OsuScore::TotalSuccessfulHits() const
{
	return _num50 + _num100 + _num300;
}

void OsuScore::computeEffectiveMissCount(const Beatmap &beatmap)
{
	// guess the number of misses + slider breaks from combo
	f32 comboBasedMissCount = 0.0f;
	f32 beatmapMaxCombo = beatmap.DifficultyAttribute(_mods, Beatmap::MaxCombo);
	if (beatmap.NumSliders() > 0)
	{
		f32 fullComboThreshold = beatmapMaxCombo - 0.1f * beatmap.NumSliders();
		if (_maxCombo < fullComboThreshold)
			comboBasedMissCount = fullComboThreshold / std::max(1, _maxCombo);
	}

	// Clamp miss count to maximum amount of possible breaks
	comboBasedMissCount = std::min(comboBasedMissCount, static_cast<f32>(_num100 + _num50 + _numMiss));

	_effectiveMissCount = std::max(static_cast<f32>(_numMiss), comboBasedMissCount);
}

void OsuScore::computeTotalValue(const Beatmap &beatmap)
{
	// Don't count scores made with supposedly unranked mods
	if ((_mods & EMods::Relax) > 0 ||
		(_mods & EMods::Relax2) > 0 ||
		(_mods & EMods::Autoplay) > 0)
	{
		_totalValue = 0;
		return;
	}

	f32 multiplier = 1.14f; // This is being adjusted to keep the final pp value scaled around what it used to be when changing things.

	if ((_mods & EMods::NoFail) > 0)
		multiplier *= std::max(0.9f, 1.0f - 0.02f * _effectiveMissCount);

	int numTotalHits = TotalHits();
	if ((_mods & EMods::SpunOut) > 0)
		multiplier *= 1.0f - std::pow(beatmap.NumSpinners() / static_cast<f32>(numTotalHits), 0.85f);

	_totalValue =
		std::pow(
			std::pow(_aimValue, 1.1f) +
				std::pow(_speedValue, 1.1f) +
				std::pow(_accuracyValue, 1.1f) +
				std::pow(_flashlightValue, 1.1),
			1.0f / 1.1f) *
		multiplier;
}

void OsuScore::computeAimValue(const Beatmap &beatmap)
{
	_aimValue = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, Beatmap::Aim) / 0.0675f) - 4.0f, 3.0f) / 100000.0f;

	int numTotalHits = TotalHits();

	f32 lengthBonus = 0.95f + 0.4f * std::min(1.0f, static_cast<f32>(numTotalHits) / 2000.0f) +
					  (numTotalHits > 2000 ? log10(static_cast<f32>(numTotalHits) / 2000.0f) * 0.5f : 0.0f);
	_aimValue *= lengthBonus;

	// Penalize misses by assessing # of misses relative to the total # of objects. Default a 3% reduction for any # of misses.
	if (_effectiveMissCount > 0)
		_aimValue *= 0.97f * std::pow(1.0f - std::pow(_effectiveMissCount / static_cast<f32>(numTotalHits), 0.775f), _effectiveMissCount);

	_aimValue *= getComboScalingFactor(beatmap);

	f32 approachRate = beatmap.DifficultyAttribute(_mods, Beatmap::AR);
	f32 approachRateFactor = 0.0f;
	if (approachRate > 10.33f)
		approachRateFactor = 0.3f * (approachRate - 10.33f);
	else if (approachRate < 8.0f)
		approachRateFactor = 0.05f * (8.0f - approachRate);

	_aimValue *= 1.0f + approachRateFactor * lengthBonus;

	// We want to give more reward for lower AR when it comes to aim and HD. This nerfs high AR and buffs lower AR.
	if ((_mods & EMods::Hidden) > 0)
		_aimValue *= 1.0f + 0.04f * (12.0f - approachRate);

	// We assume 15% of sliders in a map are difficult since there's no way to tell from the performance calculator.
	f32 estimateDifficultSliders = beatmap.NumSliders() * 0.15f;

	if (beatmap.NumSliders() > 0)
	{
		float maxCombo = beatmap.DifficultyAttribute(_mods, Beatmap::MaxCombo);
		f32 estimateSliderEndsDropped = std::min(std::max(std::min(static_cast<f32>(_num100 + _num50 + _numMiss), maxCombo - _maxCombo), 0.0f), estimateDifficultSliders);
		f32 sliderFactor = beatmap.DifficultyAttribute(_mods, Beatmap::SliderFactor);
		f32 sliderNerfFactor = (1.0f - sliderFactor) * std::pow(1.0f - estimateSliderEndsDropped / estimateDifficultSliders, 3) + sliderFactor;
		_aimValue *= sliderNerfFactor;
	}

	_aimValue *= Accuracy();
	// It is important to consider accuracy difficulty when scaling with accuracy.
	_aimValue *= 0.98f + (pow(beatmap.DifficultyAttribute(_mods, Beatmap::OD), 2) / 2500);
}

void OsuScore::computeSpeedValue(const Beatmap &beatmap)
{
	_speedValue = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, Beatmap::Speed) / 0.0675f) - 4.0f, 3.0f) / 100000.0f;

	int numTotalHits = TotalHits();

	f32 lengthBonus = 0.95f + 0.4f * std::min(1.0f, static_cast<f32>(numTotalHits) / 2000.0f) +
					  (numTotalHits > 2000 ? log10(static_cast<f32>(numTotalHits) / 2000.0f) * 0.5f : 0.0f);
	_speedValue *= lengthBonus;

	// Penalize misses by assessing # of misses relative to the total # of objects. Default a 3% reduction for any # of misses.
	if (_effectiveMissCount > 0)
		_speedValue *= 0.97f * std::pow(1.0f - std::pow(_effectiveMissCount / static_cast<f32>(numTotalHits), 0.775f), std::pow(_effectiveMissCount, 0.875f));

	_speedValue *= getComboScalingFactor(beatmap);

	f32 approachRate = beatmap.DifficultyAttribute(_mods, Beatmap::AR);
	f32 approachRateFactor = 0.0f;
	if (approachRate > 10.33f)
		approachRateFactor = 0.3f * (approachRate - 10.33f);

	_speedValue *= 1.0f + approachRateFactor * lengthBonus; // Buff for longer maps with high AR.

	// We want to give more reward for lower AR when it comes to speed and HD. This nerfs high AR and buffs lower AR.
	if ((_mods & EMods::Hidden) > 0)
		_speedValue *= 1.0f + 0.04f * (12.0f - approachRate);

	// Calculate accuracy assuming the worst case scenario
	f32 relevantTotalDiff = static_cast<f32>(numTotalHits) - beatmap.DifficultyAttribute(_mods, Beatmap::SpeedNoteCount);
	f32 relevantCountGreat = std::max(0.0f, _num300 - relevantTotalDiff);
	f32 relevantCountOk = std::max(0.0f, _num100 - std::max(0.0f, relevantTotalDiff - _num300));
	f32 relevantCountMeh = std::max(0.0f, _num50 - std::max(0.0f, relevantTotalDiff - _num300 - _num100));
	f32 relevantAccuracy = beatmap.DifficultyAttribute(_mods, Beatmap::SpeedNoteCount) == 0.0f ? 0.0f : (relevantCountGreat * 6.0f + relevantCountOk * 2.0f + relevantCountMeh) / (beatmap.DifficultyAttribute(_mods, Beatmap::SpeedNoteCount) * 6.0f);

	// Scale the speed value with accuracy and OD.
	_speedValue *= (0.95f + std::pow(beatmap.DifficultyAttribute(_mods, Beatmap::OD), 2) / 750) * std::pow((Accuracy() + relevantAccuracy) / 2.0f, (14.5f - std::max(beatmap.DifficultyAttribute(_mods, Beatmap::OD), 8.0f)) / 2);

	// Scale the speed value with # of 50s to punish doubletapping.
	_speedValue *= std::pow(0.99f, _num50 < numTotalHits / 500.0f ? 0.0f : _num50 - numTotalHits / 500.0f);
}

void OsuScore::computeAccuracyValue(const Beatmap &beatmap)
{
	// This percentage only considers HitCircles of any value - in this part of the calculation we focus on hitting the timing hit window.
	f32 betterAccuracyPercentage;

	s32 numHitObjectsWithAccuracy;
	if (beatmap.ScoreVersion() == Beatmap::EScoreVersion::ScoreV2)
	{
		numHitObjectsWithAccuracy = TotalHits();
		betterAccuracyPercentage = Accuracy();
	}
	// Either ScoreV1 or some unknown value. Let's default to previous behavior.
	else
	{
		numHitObjectsWithAccuracy = beatmap.NumHitCircles();
		if (numHitObjectsWithAccuracy > 0)
			betterAccuracyPercentage = static_cast<f32>((_num300 - (TotalHits() - numHitObjectsWithAccuracy)) * 6 + _num100 * 2 + _num50) / (numHitObjectsWithAccuracy * 6);
		else
			betterAccuracyPercentage = 0;

		// It is possible to reach a negative accuracy with this formula. Cap it at zero - zero points.
		if (betterAccuracyPercentage < 0)
			betterAccuracyPercentage = 0;
	}

	// Lots of arbitrary values from testing.
	// Considering to use derivation from perfect accuracy in a probabilistic manner - assume normal distribution.
	_accuracyValue =
		pow(1.52163f, beatmap.DifficultyAttribute(_mods, Beatmap::OD)) * pow(betterAccuracyPercentage, 24) *
		2.83f;

	// Bonus for many hitcircles - it's harder to keep good accuracy up for longer.
	_accuracyValue *= std::min(1.15f, static_cast<f32>(pow(numHitObjectsWithAccuracy / 1000.0f, 0.3f)));

	if ((_mods & EMods::Hidden) > 0)
		_accuracyValue *= 1.08f;

	if ((_mods & EMods::Flashlight) > 0)
		_accuracyValue *= 1.02f;
}

void OsuScore::computeFlashlightValue(const Beatmap &beatmap)
{
	_flashlightValue = 0.0f;

	if ((_mods & EMods::Flashlight) == 0)
		return;

	_flashlightValue = std::pow(beatmap.DifficultyAttribute(_mods, Beatmap::Flashlight), 2.0f) * 25.0f;

	int numTotalHits = TotalHits();

	// Penalize misses by assessing # of misses relative to the total # of objects. Default a 3% reduction for any # of misses.
	if (_effectiveMissCount > 0)
		_flashlightValue *= 0.97f * std::pow(1 - std::pow(_effectiveMissCount / static_cast<f32>(numTotalHits), 0.775f), std::pow(_effectiveMissCount, 0.875f));

	_flashlightValue *= getComboScalingFactor(beatmap);

	// Account for shorter maps having a higher ratio of 0 combo/100 combo flashlight radius.
	_flashlightValue *= 0.7f + 0.1f * std::min(1.0f, static_cast<f32>(numTotalHits) / 200.0f) +
						(numTotalHits > 200 ? 0.2f * std::min(1.0f, (static_cast<f32>(numTotalHits) - 200) / 200.0f) : 0.0f);

	// Scale the flashlight value with accuracy _slightly_.
	_flashlightValue *= 0.5f + Accuracy() / 2.0f;
	// It is important to also consider accuracy difficulty when doing that.
	_flashlightValue *= 0.98f + std::pow(beatmap.DifficultyAttribute(_mods, Beatmap::OD), 2.0f) / 2500.0f;
}

f32 OsuScore::getComboScalingFactor(const Beatmap &beatmap)
{
	float maxCombo = beatmap.DifficultyAttribute(_mods, Beatmap::MaxCombo);
	if (maxCombo > 0)
		return std::min(static_cast<f32>(pow(_maxCombo, 0.8f) / pow(maxCombo, 0.8f)), 1.0f);
	return 1.0f;
}
PP_NAMESPACE_END
