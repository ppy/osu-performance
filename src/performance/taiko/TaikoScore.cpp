#include <pp/common.h>
#include <pp/performance/taiko/TaikoScore.h>

PP_NAMESPACE_BEGIN

TaikoScore::TaikoScore(
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
	computeStrainValue(beatmap);
	computeAccValue(beatmap);

	computeTotalValue();
}

f32 TaikoScore::TotalValue() const
{
	return _totalValue;
}

void TaikoScore::computeTotalValue()
{
	// Don't count scores made with supposedly unranked mods
	if ((_mods & EMods::Relax) > 0 ||
		(_mods & EMods::Relax2) > 0 ||
		(_mods & EMods::Autoplay) > 0)
	{
		_totalValue = 0;
		return;
	}

	// Custom multipliers for NoFail and SpunOut.
	f32 multiplier = 1.1f; // This is being adjusted to keep the final pp value scaled around what it used to be when changing things

	if ((_mods & EMods::NoFail) > 0)
		multiplier *= 0.90f;

	if ((_mods & EMods::Hidden) > 0)
		multiplier *= 1.10f;

	_totalValue =
		std::pow(
			std::pow(_strainValue, 1.1f) +
			std::pow(_accValue, 1.1f), 1.0f / 1.1f
		) * multiplier;
}

void TaikoScore::computeStrainValue(const Beatmap& beatmap)
{
	_strainValue = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, Beatmap::Strain) / 0.0075f) - 4.0f, 2.0f) / 100000.0f;

	// Longer maps are worth more
	f32 lengthBonus = 1 + 0.1f * std::min(1.0f, static_cast<f32>(TotalHits()) / 1500.0f);
	_strainValue *= lengthBonus;

	// Penalize misses exponentially. This mainly fixes tag4 maps and the likes until a per-hitobject solution is available
	_strainValue *= pow(0.985f, _numMiss);

	// Combo scaling
	float maxCombo = beatmap.DifficultyAttribute(_mods, Beatmap::MaxCombo);
	if (maxCombo > 0)
		_strainValue *= std::min(static_cast<f32>(pow(_maxCombo, 0.5f) / pow(maxCombo, 0.5f)), 1.0f);

	if ((_mods & EMods::Hidden) > 0)
		_strainValue *= 1.025f;

	if ((_mods & EMods::Flashlight) > 0)
		// Apply length bonus again if flashlight is on simply because it becomes a lot harder on longer maps.
		_strainValue *= 1.05f * lengthBonus;

	// Scale the speed value with accuracy _slightly_
	_strainValue *= Accuracy();
}

void TaikoScore::computeAccValue(const Beatmap& beatmap)
{
	f32 hitWindow300 = beatmap.DifficultyAttribute(_mods, Beatmap::HitWindow300);
	if (hitWindow300 <= 0)
	{
		_accValue = 0;
		return;
	}

	// Lots of arbitrary values from testing.
	// Considering to use derivation from perfect accuracy in a probabilistic manner - assume normal distribution
	_accValue = pow(150.0f / hitWindow300, 1.1f) * pow(Accuracy(), 15) * 22.0f;

	// Bonus for many hitcircles - it's harder to keep good accuracy up for longer
	_accValue *= std::min<f32>(1.15f, pow(static_cast<f32>(TotalHits()) / 1500.0f, 0.3f));
}

f32 TaikoScore::Accuracy() const
{
	if (TotalHits() == 0)
		return 0;

	return
		Clamp(static_cast<f32>(_num100 * 150 + _num300 * 300)
		/ (TotalHits() * 300), 0.0f, 1.0f);
}

s32 TaikoScore::TotalHits() const
{
	return _num50 + _num100 + _num300 + _numMiss;
}

s32 TaikoScore::TotalSuccessfulHits() const
{
	return _num50 + _num100 + _num300;
}

PP_NAMESPACE_END
