#include <PrecompiledHeader.h>

#include "CatchTheBeatScore.h"
#include "SharedEnums.h"

CCatchTheBeatScore::CCatchTheBeatScore(
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
	// Don't count scores made with supposedly unranked mods
	if ((_mods & EMods::Relax) > 0 ||
		(_mods & EMods::Relax2) > 0 ||
		(_mods & EMods::Autoplay) > 0)
	{
		_value = 0;
		return;
	}

	// We are heavily relying on aim in catch the beat
	_value = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, CBeatmap::Aim) / 0.0049f) - 4.0f, 2.0f) / 100000.0f;

	// Longer maps are worth more. "Longer" means how many hits there are which can contribute to combo
	int amountTotalHits = TotalComboHits();

	// Longer maps are worth more
	f32 lengthBonus =
		0.95f + 0.4f * std::min<f32>(1.0f, static_cast<f32>(amountTotalHits) / 3000.0f) +
		(amountTotalHits > 3000 ? log10(static_cast<f32>(amountTotalHits) / 3000.0f) * 0.5f : 0.0f);

	// Longer maps are worth more
	_value *= lengthBonus;

	// Penalize misses exponentially. This mainly fixes tag4 maps and the likes until a per-hitobject solution is available
	_value *= pow(0.97f, _amountMiss);

	// Combo scaling
	float beatmapMaxCombo = beatmap.DifficultyAttribute(_mods, CBeatmap::MaxCombo);
	if (beatmapMaxCombo > 0)
		_value *= std::min<f32>(pow(static_cast<f32>(_maxCombo), 0.8f) / pow(beatmapMaxCombo, 0.8f), 1.0f);

	f32 approachRate = beatmap.DifficultyAttribute(_mods, CBeatmap::AR);
	f32 approachRateFactor = 1.0f;
	if (approachRate > 9.0f)
		approachRateFactor += 0.1f * (approachRate - 9.0f); // 10% for each AR above 9
	else if (approachRate < 8.0f)
		approachRateFactor += 0.025f * (8.0f - approachRate); // 2.5% for each AR below 8

	_value *= approachRateFactor;

	if ((_mods & EMods::Hidden) > 0)
		// Hiddens gives nothing on max approach rate, and more the lower it is
		_value *= 1.05f + 0.075f * (10.0f - std::min(10.0f, approachRate)); // 7.5% for each AR below 10

	if ((_mods & EMods::Flashlight) > 0)
		// Apply length bonus again if flashlight is on simply because it becomes a lot harder on longer maps.
		_value *= 1.35f * lengthBonus;

	// Scale the aim value with accuracy _slightly_
	_value *= pow(Accuracy(), 5.5f);

	// Custom multipliers for NoFail and SpunOut.
	if ((_mods & EMods::NoFail) > 0)
		_value *= 0.90f;

	if ((_mods & EMods::SpunOut) > 0)
		_value *= 0.95f;
}

f32 CCatchTheBeatScore::TotalValue() const
{
	return _value;
}

f32 CCatchTheBeatScore::Accuracy() const
{
	if (TotalHits() == 0)
		return 0;

	return clamp(static_cast<f32>(TotalSuccessfulHits()) / TotalHits(), 0.0f, 1.0f);
}

s32 CCatchTheBeatScore::TotalHits() const
{
	return _amount50 + _amount100 + _amount300 + _amountMiss + _amountKatu;
}

s32 CCatchTheBeatScore::TotalSuccessfulHits() const
{
	return _amount50 + _amount100 + _amount300;
}

s32 CCatchTheBeatScore::TotalComboHits() const
{
	return _amount300 + _amount100 + _amountMiss;
}
