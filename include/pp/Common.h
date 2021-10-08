#pragma once

#include <pp/shared/Types.h>

// Common logging stuff
#include <tinylogger/tinylogger.h>
#include <StrFormat.h>

#include <vector>

// We use a helper macro such that our namespace does not cause indentation.
#define PP_NAMESPACE_BEGIN namespace pp {
#define PP_NAMESPACE_END }

PP_NAMESPACE_BEGIN

class Exception
{
public:
	Exception(const std::string& file, s32 line, const std::string& description)
	: _file{file}, _line{line}, _description{description} {}
	~Exception() = default;

	void Log() const { tlog::error() << StrFormat("{0}:{1} - {2}", _file, _line, _description); }

	std::string Description() const { return _description; }

	std::string File() const { return _file; }

	s32 Line() const { return _line; }

protected:
	std::string _file;
	s32 _line;
	std::string _description;
};

#define DEFINE_EXCEPTION(x)                                                  \
	class x : public Exception                                               \
	{                                                                        \
	public:                                                                  \
		x(const std::string &file, s32 line, const std::string &description) \
			: Exception{file, line, description} { Log(); }                  \
	}

#define SRC_POS __FILE__,__LINE__

// std::string operations
std::vector<std::string> Split(std::string text, const std::string& delim);
std::string Join(const std::vector<std::string>& words, const std::string& delim);
std::string ToLower(std::string str);
std::string ToUpper(std::string str);

// Math operations
template <class T>
const T Clamp(const T& value, const T& low, const T& high)
{
	return std::min(std::max(value, low), high);
}

// Common enums
enum EMods : u32
{
	Nomod = 0,
	NoFail = 1 << 0,
	Easy = 1 << 1,
	TouchDevice = 1 << 2,
	Hidden = 1 << 3,
	HardRock = 1 << 4,
	SuddenDeath = 1 << 5,
	DoubleTime = 1 << 6,
	Relax = 1 << 7,
	HalfTime = 1 << 8,
	Nightcore = 1 << 9,
	Flashlight = 1 << 10,
	Autoplay = 1 << 11,
	SpunOut = 1 << 12,
	Relax2 = 1 << 13,
	Perfect = 1 << 14,
	Key4 = 1 << 15,
	Key5 = 1 << 16,
	Key6 = 1 << 17,
	Key7 = 1 << 18,
	Key8 = 1 << 19,
	FadeIn = 1 << 20,
	Random = 1 << 21,
	Cinema = 1 << 22,
	Target = 1 << 23,
	Key9 = 1 << 24,
	Key10 = 1 << 25,
	Key1 = 1 << 26,
	Key3 = 1 << 27,
	Key2 = 1 << 28,
	LastMod = 1 << 29,
	keyMod = Key1 | Key2 | Key3 | Key4 | Key5 | Key6 | Key7 | Key8 | Key9 | Key10,
	KeyModUnranked = Key1 | Key2 | Key3 | Key9 | Key10,
	FreeModAllowed = NoFail | Easy | Hidden | HardRock | SuddenDeath | Flashlight | FadeIn | Relax | Relax2 | SpunOut | keyMod,
	ScoreIncreaseMods = Hidden | HardRock | DoubleTime | Flashlight | FadeIn
};

inline EMods MaskRelevantDifficultyMods(EMods mods)
{
	return static_cast<EMods>(mods & (DoubleTime | HalfTime | HardRock | Easy | Flashlight | keyMod));
}

std::string ToString(EMods mods);

enum class EGamemode
{
	Osu,
	Taiko,
	Catch,
	Mania,
};

std::string GamemodeSuffix(EGamemode gamemode);
std::string GamemodeName(EGamemode gamemode);
std::string GamemodeTag(EGamemode gamemode);
EGamemode ToGamemode(std::string modeString);

PP_NAMESPACE_END
