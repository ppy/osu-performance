#include <pp/Common.h>
#include <pp/performance/Beatmap.h>

PP_NAMESPACE_BEGIN

const std::unordered_map<std::string, Beatmap::EDifficultyAttributeType> Beatmap::s_difficultyAttributes{
	{"Aim",              Aim},
	{"Speed",            Speed},
	{"OD",               OD},
	{"AR",               AR},
	{"Max combo",        MaxCombo},
	{"Strain",           Strain},
	{"Hit window 300",   HitWindow300},
	{"Score multiplier", ScoreMultiplier},
};

Beatmap::Beatmap(s32 id)
: _id{id}
{
}

f32 Beatmap::DifficultyAttribute(EMods mods, EDifficultyAttributeType type) const
{
	mods = maskRelevantDifficultyMods(mods);

	if (_difficulty.count(mods) == 0 || _difficulty.at(mods).count(type) == 0)
		return 0.0f;
	else
		return _difficulty.at(mods).at(type);
}

void Beatmap::SetDifficultyAttribute(EMods mods, EDifficultyAttributeType type, f32 value)
{
	mods = maskRelevantDifficultyMods(mods);
	_difficulty[mods][type] = value;
}

PP_NAMESPACE_END
