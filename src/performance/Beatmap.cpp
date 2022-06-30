#include <pp/Common.h>
#include <pp/performance/Beatmap.h>

PP_NAMESPACE_BEGIN

const std::unordered_map<std::string, Beatmap::EDifficultyAttributeType> Beatmap::s_difficultyAttributes{
	{"Aim", Aim},
	{"Speed", Speed},
	{"OD", OD},
	{"AR", AR},
	{"Max combo", MaxCombo},
	{"Strain", Strain},
	{"Hit window 300", HitWindow300},
	{"Score multiplier", ScoreMultiplier},
	{"Flashlight", Flashlight},
	{"Slider factor", SliderFactor},
	{"Speed note count", SpeedNoteCount},
};

Beatmap::Beatmap(s32 id)
	: _id{id}
{
}

f32 Beatmap::DifficultyAttribute(EMods mods, EDifficultyAttributeType type) const
{
	auto difficultyIt = _difficulty.find(MaskRelevantDifficultyMods(_mode, mods));
	return difficultyIt == std::end(_difficulty) ? 0.0f : difficultyIt->second[type];
}

void Beatmap::SetDifficultyAttribute(EMods mods, EDifficultyAttributeType type, f32 value)
{
	_difficulty[MaskRelevantDifficultyMods(_mode, mods)][type] = value;
}

PP_NAMESPACE_END
