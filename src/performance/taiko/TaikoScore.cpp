#include <pp/Common.h>
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
	const Beatmap &beatmap) : Score{scoreId, mode, userId, beatmapId, score, maxCombo, num300, num100, num50, numMiss, numGeki, numKatu, mods}
{
	computeDifficultyValue(beatmap);
	computeAccuracyValue(beatmap);

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

	f32 multiplier = 1.1f; // This is being adjusted to keep the final pp value scaled around what it used to be when changing things

	if ((_mods & EMods::NoFail) > 0)
		multiplier *= 0.90f;

	if ((_mods & EMods::Hidden) > 0)
		multiplier *= 1.10f;

	_totalValue =
		std::pow(
			std::pow(_difficultyValue, 1.1f) +
				std::pow(_accuracyValue, 1.1f),
			1.0f / 1.1f) *
		multiplier;
}

void TaikoScore::computeDifficultyValue(const Beatmap &beatmap)
{
	_difficultyValue = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, Beatmap::Strain) / 0.0075f) - 4.0f, 2.0f) / 100000.0f;

	f32 lengthBonus = 1 + 0.1f * std::min(1.0f, static_cast<f32>(TotalHits()) / 1500.0f);
	_difficultyValue *= lengthBonus;

	_difficultyValue *= pow(0.985f, _numMiss);

	if ((_mods & EMods::Hidden) > 0)
		_difficultyValue *= 1.025f;

	if ((_mods & EMods::Flashlight) > 0)
		_difficultyValue *= 1.05f * lengthBonus;

	_difficultyValue *= Accuracy();
}

void TaikoScore::computeAccuracyValue(const Beatmap &beatmap)
{
	f32 hitWindow300 = beatmap.DifficultyAttribute(_mods, Beatmap::HitWindow300);
	if (hitWindow300 <= 0)
	{
		_accuracyValue = 0;
		return;
	}

	_accuracyValue = pow(150.0f / hitWindow300, 1.1f) * pow(Accuracy(), 15) * 22.0f;

	// Bonus for many objects - it's harder to keep good accuracy up for longer
	_accuracyValue *= std::min<f32>(1.15f, pow(static_cast<f32>(TotalHits()) / 1500.0f, 0.3f));
}

f32 TaikoScore::Accuracy() const
{
	if (TotalHits() == 0)
		return 0;

	return Clamp(static_cast<f32>(_num100 * 150 + _num300 * 300) / (TotalHits() * 300), 0.0f, 1.0f);
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
