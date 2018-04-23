#include <PrecompiledHeader.h>

#include "SharedEnums.h"
#include "TaikoScore.h"

CTaikoScore::CTaikoScore(
	s64 scoreId,
	EGamemode mode,
	s32 userId,
	s32 beatmapId,
	s32 score,
	s32 maxCombo,
	s32 amount300,
	s32 amount100,
	s32 amount50,
	s32 amountMiss,
	s32 amountGeki,
	s32 amountKatu,
	EMods mods,
	const CBeatmap& beatmap
) : CScore{scoreId, mode, userId, beatmapId, score, maxCombo, amount300, amount100, amount50, amountMiss, amountGeki, amountKatu, mods}
{
	ComputeStrainValue(beatmap);
	ComputeAccValue(beatmap);

	ComputeTotalValue();
}

f32 CTaikoScore::TotalValue() const
{
	return _totalValue;
}

void CTaikoScore::ComputeTotalValue()
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

void CTaikoScore::ComputeStrainValue(const CBeatmap& beatmap)
{
	_strainValue = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, CBeatmap::Strain) / 0.0075f) - 4.0f, 2.0f) / 100000.0f;

	// Longer maps are worth more
	f32 lengthBonus = 1 + 0.1f * std::min(1.0f, static_cast<f32>(TotalHits()) / 1500.0f);
	_strainValue *= lengthBonus;

	// Penalize misses exponentially. This mainly fixes tag4 maps and the likes until a per-hitobject solution is available
	_strainValue *= pow(0.985f, _amountMiss);

	// Combo scaling
	float maxCombo = beatmap.DifficultyAttribute(_mods, CBeatmap::MaxCombo);
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

void CTaikoScore::ComputeAccValue(const CBeatmap& beatmap)
{
	f32 hitWindow300 = beatmap.DifficultyAttribute(_mods, CBeatmap::HitWindow300);
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

f32 CTaikoScore::Accuracy() const
{
	if (TotalHits() == 0)
		return 0;

	return
		clamp(static_cast<f32>(_amount100 * 150 + _amount300 * 300)
		/ (TotalHits() * 300), 0.0f, 1.0f);
}

s32 CTaikoScore::TotalHits() const
{
	return _amount50 + _amount100 + _amount300 + _amountMiss;
}

s32 CTaikoScore::TotalSuccessfulHits() const
{
	return _amount50 + _amount100 + _amount300;
}
