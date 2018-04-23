#include <PrecompiledHeader.h>

#include "Beatmap.h"
#include "SharedEnums.h"

const std::unordered_map<std::string, CBeatmap::EDifficultyAttributeType> CBeatmap::s_difficultyAttributes{
	{"Aim",              Aim},
	{"Speed",            Speed},
	{"OD",               OD},
	{"AR",               AR},
	{"Max combo",        MaxCombo},
	{"Strain",           Strain},
	{"Hit window 300",   HitWindow300},
	{"Score multiplier", ScoreMultiplier},
};

CBeatmap::CBeatmap(s32 id)
: _id{id}
{
}

f32 CBeatmap::DifficultyAttribute(EMods mods, EDifficultyAttributeType type) const
{
	mods = MaskRelevantDifficultyMods(mods);

	if (_difficulty.count(mods) == 0 || _difficulty.at(mods).count(type) == 0)
		return 0.0f;
	else
		return _difficulty.at(mods).at(type);
}

void CBeatmap::SetDifficultyAttribute(EMods mods, EDifficultyAttributeType type, f32 value)
{
	mods = MaskRelevantDifficultyMods(mods);
	_difficulty[mods][type] = value;
}
