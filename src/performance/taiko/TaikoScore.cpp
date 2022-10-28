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
	// The effectiveMissCount is calculated by gaining a ratio for totalSuccessfulHits and increasing the miss penalty for shorter object counts lower than 1000.
	if (TotalSuccessfulHits() > 0)
		_effectiveMissCount = std::max(1.0f, 1000.0f / static_cast<f32>(TotalSuccessfulHits())) * static_cast<f32>(_numMiss);

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

	f32 multiplier = 1.13f; // This is being adjusted to keep the final pp value scaled around what it used to be when changing things

	if ((_mods & EMods::Hidden) > 0)
		multiplier *= 1.075f;

	if ((_mods & EMods::Easy) > 0)
		multiplier *= 0.975f;

	_totalValue =
		std::pow(
			std::pow(_difficultyValue, 1.1f) +
				std::pow(_accuracyValue, 1.1f),
			1.0f / 1.1f) *
		multiplier;
}

void TaikoScore::computeDifficultyValue(const Beatmap &beatmap)
{
	_difficultyValue = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, Beatmap::Strain) / 0.115f) - 4.0f, 2.25f) / 1150.0f;

	f32 lengthBonus = 1 + 0.1f * std::min(1.0f, static_cast<f32>(TotalHits()) / 1500.0f);
	_difficultyValue *= lengthBonus;

	_difficultyValue *= pow(0.986f, _effectiveMissCount);

	if ((_mods & EMods::Easy) > 0)
		_difficultyValue *= 0.985f;

	if ((_mods & EMods::Hidden) > 0 && beatmap.PlayMode() == EGamemode::Taiko)
		_difficultyValue *= 1.025f;

	if ((_mods & EMods::HardRock) > 0)
		_difficultyValue *= 1.050f;

	if ((_mods & EMods::Flashlight) > 0)
		_difficultyValue *= 1.050f * lengthBonus;

	_difficultyValue *= std::pow(Accuracy(), 2.0f);
}

void TaikoScore::computeAccuracyValue(const Beatmap &beatmap)
{
	f32 hitWindow300 = beatmap.DifficultyAttribute(_mods, Beatmap::HitWindow300);
	if (hitWindow300 <= 0)
	{
		_accuracyValue = 0;
		return;
	}

	_accuracyValue = pow(60.0f / hitWindow300, 1.1f) * pow(Accuracy(), 8.0f) * std::pow(beatmap.DifficultyAttribute(_mods, Beatmap::Strain), 0.4f) * 27.0f;

	f32 lengthBonus = std::min(1.15f, std::pow(static_cast<f32>(TotalHits()) / 1500.0f, 0.3f));
	_accuracyValue *= lengthBonus;

	// Slight HDFL Bonus for accuracy. A clamp is used to prevent against negative values.
	if ((_mods & (EMods::Hidden | EMods::Flashlight)) == (EMods::Hidden | EMods::Flashlight) && beatmap.PlayMode() == EGamemode::Taiko)
		_accuracyValue *= std::max(1.0f, 1.1f * lengthBonus);
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
