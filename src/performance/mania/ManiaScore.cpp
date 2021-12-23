#include <pp/Common.h>
#include <pp/performance/mania/ManiaScore.h>

PP_NAMESPACE_BEGIN

ManiaScore::ManiaScore(
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
	computeDifficultyValue(beatmap);
	computeAccuracyValue(beatmap);

	computeTotalValue();
}

f32 ManiaScore::TotalValue() const
{
	return _totalValue;
}

void ManiaScore::computeTotalValue()
{
	// Don't count scores made with supposedly unranked mods
	if ((_mods & EMods::Relax) > 0 ||
		(_mods & EMods::Relax2) > 0 ||
		(_mods & EMods::Autoplay) > 0)
	{
		_totalValue = 0;
		return;
	}

	/*
		Arbitrary initial value for scaling pp in order to standardize distributions across game modes.
		The specific number has no intrinsic meaning and can be adjusted as needed.
	*/
	f32 multiplier = 0.8f;

	if ((_mods & EMods::NoFail) > 0)
		multiplier *= 0.90f;

	if ((_mods & EMods::SpunOut) > 0)
		multiplier *= 0.95f;

	if ((_mods & EMods::Easy) > 0)
		multiplier *= 0.50f;

	_totalValue =
		std::pow(
			std::pow(_difficultyValue, 1.1f) +
				std::pow(_accuracyValue, 1.1f),
			1.0f / 1.1f) *
		multiplier;
}

void ManiaScore::computeDifficultyValue(const Beatmap &beatmap)
{
	// Scale score up, so it's comparable to other keymods
	f32 scoreMultiplier = beatmap.DifficultyAttribute(_mods, Beatmap::ScoreMultiplier);

	if (scoreMultiplier <= 0)
	{
		_difficultyValue = 0;
		return;
	}

	_score *= static_cast<s32>(1.0f / scoreMultiplier); // We don't really mind rounding errors with such small magnitude.

	_difficultyValue = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, Beatmap::Strain) / 0.2f) - 4.0f, 2.2f) / 135.0f;

	_difficultyValue *= 1 + 0.1f * std::min(1.0f, static_cast<f32>(TotalHits()) / 1500.0f);

	if (_score <= 500000)
		_difficultyValue = 0;
	else if (_score <= 600000)
		_difficultyValue *= static_cast<f32>(_score - 500000) / 100000.0f * 0.3f;
	else if (_score <= 700000)
		_difficultyValue *= 0.3f + static_cast<f32>(_score - 600000) / 100000.0f * 0.25f;
	else if (_score <= 800000)
		_difficultyValue *= 0.55f + static_cast<f32>(_score - 700000) / 100000.0f * 0.20f;
	else if (_score <= 900000)
		_difficultyValue *= 0.75f + static_cast<f32>(_score - 800000) / 100000.0f * 0.15f;
	else
		_difficultyValue *= 0.90f + static_cast<f32>(_score - 900000) / 100000.0f * 0.1f;
}

void ManiaScore::computeAccuracyValue(const Beatmap &beatmap)
{
	f32 hitWindow300 = beatmap.DifficultyAttribute(_mods, Beatmap::HitWindow300);
	if (hitWindow300 <= 0)
	{
		_accuracyValue = 0;
		return;
	}

	// Lots of arbitrary values from testing.
	// Considering to use derivation from perfect accuracy in a probabilistic manner - assume normal distribution
	_accuracyValue = std::max(0.0f, 0.2f - ((hitWindow300 - 34) * 0.006667f)) * _difficultyValue * pow((std::max(0.0f, static_cast<f32>(_score - 960000)) / 40000.0f), 1.1f);
}

f32 ManiaScore::Accuracy() const
{
	if (TotalHits() == 0)
		return 0;

	return Clamp(static_cast<f32>(_num50 * 50 + _num100 * 100 + _numKatu * 200 + (_num300 + _numGeki) * 300) / (TotalHits() * 300), 0.0f, 1.0f);
}

s32 ManiaScore::TotalHits() const
{
	return _num50 + _num100 + _num300 + _numMiss + _numGeki + _numKatu;
}

s32 ManiaScore::TotalSuccessfulHits() const
{
	return _num50 + _num100 + _num300 + _numGeki + _numKatu;
}

PP_NAMESPACE_END
