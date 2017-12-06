#include <PrecompiledHeader.h>

#include "StandardScore.h"
#include "SharedEnums.h"


using namespace SharedEnums;


CStandardScore::CStandardScore(
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
	SharedEnums::EMods mods,
	const CBeatmap& beatmap)
	: CScore{scoreId, mode, userId, beatmapId, score, maxCombo, amount300, amount100, amount50, amountMiss, amountGeki, amountKatu, mods}
{
	ComputeAimValue(beatmap);
	ComputeSpeedValue(beatmap);
	ComputeAccValue(beatmap);

	ComputeTotalValue();
}


f32 CStandardScore::TotalValue() const
{
	return _totalValue;
}

f32 CStandardScore::Accuracy() const
{
	if(TotalHits() == 0)
	{
		return 0;
	}

	return clamp(static_cast<f32>(_amount50 * 50 + _amount100 * 100 + _amount300 * 300)
				 / (TotalHits() * 300), 0.0f, 1.0f);
}

s32 CStandardScore::TotalHits() const
{
	return _amount50 + _amount100 + _amount300 + _amountMiss;
}

s32 CStandardScore::TotalSuccessfulHits() const
{
	return _amount50 + _amount100 + _amount300;
}

void CStandardScore::ComputeTotalValue()
{
	// Don't count scores made with supposedly unranked mods
	if((_mods & EMods::Relax) > 0 ||
	   (_mods & EMods::Relax2) > 0 ||
	   (_mods & EMods::Autoplay) > 0)
	{
		_totalValue = 0;
		return;
	}


	// Custom multipliers for NoFail and SpunOut.
	f32 multiplier = 1.12f; // This is being adjusted to keep the final pp value scaled around what it used to be when changing things

	if((_mods & EMods::NoFail) > 0)
	{
		multiplier *= 0.90f;
	}

	if((_mods & EMods::SpunOut) > 0)
	{
		multiplier *= 0.95f;
	}

	_totalValue =
		std::pow(
			std::pow(_aimValue, 1.1f) +
			std::pow(_speedValue, 1.1f) +
			std::pow(_accValue, 1.1f), 1.0f / 1.1f
		) * multiplier;
}

void CStandardScore::ComputeAimValue(const CBeatmap& beatmap)
{
	f32 rawAim = beatmap.DifficultyAttribute(_mods, CBeatmap::Aim);

	if((_mods & EMods::TouchDevice) > 0)
		rawAim = pow(rawAim, 0.8f);

	_aimValue = pow(5.0f * std::max(1.0f, rawAim / 0.0675f) - 4.0f, 3.0f) / 100000.0f;

	int amountTotalHits = TotalHits();

	// Longer maps are worth more
	f32 LengthBonus = 0.95f + 0.4f * std::min(1.0f, static_cast<f32>(amountTotalHits) / 2000.0f) +
		(amountTotalHits > 2000 ? log10(static_cast<f32>(amountTotalHits) / 2000.0f) * 0.5f : 0.0f);

	_aimValue *= LengthBonus;

	// Penalize misses exponentially. This mainly fixes tag4 maps and the likes until a per-hitobject solution is available
	_aimValue *= pow(0.97f, _amountMiss);

	// Combo scaling
	float maxCombo = beatmap.DifficultyAttribute(_mods, CBeatmap::MaxCombo);
	if(maxCombo > 0)
	{
		_aimValue *=
			std::min(static_cast<f32>(pow(_maxCombo, 0.8f) / pow(maxCombo, 0.8f)), 1.0f);
	}
	
	f32 approachRate = beatmap.DifficultyAttribute(_mods, CBeatmap::AR);
	f32 approachRateFactor = 1.0f;
	if(approachRate > 10.33f)
	{
		approachRateFactor += 0.45f * (approachRate - 10.33f);
	}
	else if(approachRate < 8.0f)
	{
		// HD is worth more with lower ar!
		if((_mods & EMods::Hidden) > 0)
		{
			approachRateFactor += 0.02f * (8.0f - approachRate);
		}
		else
		{
			approachRateFactor += 0.01f * (8.0f - approachRate);
		}
	}

	_aimValue *= approachRateFactor;

	if((_mods & EMods::Hidden) > 0)
	{
		_aimValue *= 1.18f;
	}

	if((_mods & EMods::Flashlight) > 0)
	{
		// Apply length bonus again if flashlight is on simply because it becomes a lot harder on longer maps.
		_aimValue *= 1.45f * LengthBonus;
	}

	// Scale the aim value with accuracy _slightly_
	_aimValue *= 0.5f + Accuracy() / 2.0f;
	// It is important to also consider accuracy difficulty when doing that
	_aimValue *= 0.98f + (pow(beatmap.DifficultyAttribute(_mods, CBeatmap::OD), 2) / 2500);
}

void CStandardScore::ComputeSpeedValue(const CBeatmap& beatmap)
{
	_speedValue = pow(5.0f * std::max(1.0f, beatmap.DifficultyAttribute(_mods, CBeatmap::Speed) / 0.0675f) - 4.0f, 3.0f) / 100000.0f;

	int amountTotalHits = TotalHits();

	// Longer maps are worth more
	_speedValue *=
		0.95f + 0.4f * std::min(1.0f, static_cast<f32>(amountTotalHits) / 2000.0f) +
		(amountTotalHits > 2000 ? log10(static_cast<f32>(amountTotalHits) / 2000.0f) * 0.5f : 0.0f);

	// Penalize misses exponentially. This mainly fixes tag4 maps and the likes until a per-hitobject solution is available
	_speedValue *= pow(0.97f, _amountMiss);

	// Combo scaling
	float maxCombo = beatmap.DifficultyAttribute(_mods, CBeatmap::MaxCombo);
	if(maxCombo > 0)
	{
		_speedValue *=
			std::min(static_cast<f32>(pow(_maxCombo, 0.8f) / pow(maxCombo, 0.8f)), 1.0f);
	}
	

	// Scale the speed value with accuracy _slightly_
	_speedValue *= 0.5f + Accuracy() / 2.0f;
	// It is important to also consider accuracy difficulty when doing that
	_speedValue *= 0.98f + (pow(beatmap.DifficultyAttribute(_mods, CBeatmap::OD), 2) / 2500);
}

void CStandardScore::ComputeAccValue(const CBeatmap& beatmap)
{
	// This percentage only considers HitCircles of any value - in this part of the calculation we focus on hitting the timing hit window
	f32 betterAccuracyPercentage;

	s32 amountHitObjectsWithAccuracy;
	if(beatmap.ScoreVersion() == CBeatmap::EScoreVersion::ScoreV2)
	{
		amountHitObjectsWithAccuracy = TotalHits();
		betterAccuracyPercentage = Accuracy();
	}
	// Either ScoreV1 or some unknown value. Let's default to previous behavior.
	else
	{
		amountHitObjectsWithAccuracy = beatmap.AmountHitCircles();
		if(amountHitObjectsWithAccuracy > 0)
		{
			betterAccuracyPercentage =
				static_cast<f32>((_amount300 - (TotalHits() - amountHitObjectsWithAccuracy)) * 6 + _amount100 * 2 + _amount50) / (amountHitObjectsWithAccuracy * 6);
		}
		else
		{
			betterAccuracyPercentage = 0;
		}

		// It is possible to reach a negative accuracy with this formula. Cap it at zero - zero points
		if(betterAccuracyPercentage < 0)
		{
			betterAccuracyPercentage = 0;
		}
	}

	// Lots of arbitrary values from testing.
	// Considering to use derivation from perfect accuracy in a probabilistic manner - assume normal distribution
	_accValue =
		pow(1.52163f, beatmap.DifficultyAttribute(_mods, CBeatmap::OD)) * pow(betterAccuracyPercentage, 24) *
		2.83f;

	// Bonus for many hitcircles - it's harder to keep good accuracy up for longer
	_accValue *= std::min(1.15f, static_cast<f32>(pow(amountHitObjectsWithAccuracy / 1000.0f, 0.3f)));

	if((_mods & EMods::Hidden) > 0)
	{
		_accValue *= 1.02f;
	}

	if((_mods & EMods::Flashlight) > 0)
	{
		_accValue *= 1.02f;
	}
}
