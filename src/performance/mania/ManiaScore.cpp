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
	f32 multiplier = 8.0f;

	if ((_mods & EMods::NoFail) > 0)
		multiplier *= 0.75f;

	if ((_mods & EMods::SpunOut) > 0)
		multiplier *= 0.95f;

	if ((_mods & EMods::Easy) > 0)
		multiplier *= 0.50f;

	_totalValue = _difficultyValue * multiplier;
}

void ManiaScore::computeDifficultyValue(const Beatmap &beatmap)
{
	_difficultyValue = std::pow(std::max(beatmap.DifficultyAttribute(_mods, Beatmap::Strain) - 0.15f, 0.05f), 2.2f) // Star rating to pp curve
					   * std::max(0.0f, 5.0f * customAccuracy() - 4.0f)												// From 80% accuracy, 1/20th of total pp is awarded per additional 1% accuracy
					   * (1.0f + 0.1f * std::min(1.0f, static_cast<f32>(TotalHits()) / 1500.0f));					// Length bonus, capped at 1500 notes
}

f32 ManiaScore::customAccuracy() const
{
	if (TotalHits() == 0)
		return 0;

	return static_cast<f32>(_numGeki * 320 + _num300 * 300 + _numKatu * 200 + _num100 * 100 + _num50 * 50) / (TotalHits() * 320);
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
