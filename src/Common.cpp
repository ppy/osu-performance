#include <pp/Common.h>

#include <string>
#include <vector>

PP_NAMESPACE_BEGIN

std::vector<std::string> Split(std::string text, const std::string& delim) {
    std::vector<std::string> result;
    while (true) {
        size_t begin = text.find_last_of(delim);
        if (begin == std::string::npos) {
            result.emplace_back(text);
            return result;
        } else {
            result.emplace_back(text.substr(begin + 1));
            text.resize(begin);
        }
    }

    return result;
}

std::string ToLower(std::string str) {
    transform(begin(str), end(str), begin(str), [](unsigned char c) { return (char)tolower(c); });
    return str;
}

std::string ToUpper(std::string str) {
    transform(begin(str), end(str), begin(str), [](unsigned char c) { return (char)toupper(c); });
    return str;
}

std::string GamemodeSuffix(EGamemode gamemode)
{
	switch (gamemode)
	{
	case EGamemode::Osu:   return "";
	case EGamemode::Taiko: return "_taiko";
	case EGamemode::Catch: return "_fruits";
	case EGamemode::Mania: return "_mania";
	default:
		throw Exception{SRC_POS, StrFormat("Unknown gamemode requested. ({0})", gamemode)};
	}
}

std::string GamemodeName(EGamemode gamemode)
{
	switch (gamemode)
	{
	case EGamemode::Osu:   return "osu!";
	case EGamemode::Taiko: return "osu!taiko";
	case EGamemode::Catch: return "osu!catch";
	case EGamemode::Mania: return "osu!mania";
	default:
		throw Exception{SRC_POS, StrFormat("Unknown gamemode requested. ({0})", gamemode)};
	}
}

std::string GamemodeTag(EGamemode gamemode)
{
	switch (gamemode)
	{
	case EGamemode::Osu:   return "osu";
	case EGamemode::Taiko: return "taiko";
	case EGamemode::Catch: return "catch_the_beat";
	case EGamemode::Mania: return "osu_mania";
	default:
		throw Exception{SRC_POS, StrFormat("Unknown gamemode requested. ({0})", gamemode)};
	}
}

EGamemode ToGamemode(std::string modeString)
{
	modeString = ToLower(modeString);
	if (modeString == "osu" || modeString == "osu!" || modeString == "standard")
		return EGamemode::Osu;
	else if (modeString == "taiko" || modeString == "osu!taiko")
		return EGamemode::Taiko;
	else if (modeString == "catch" || modeString == "osu!catch" || modeString == "fruits" || modeString == "catchthebeat" || modeString == "catch the beat")
		return EGamemode::Catch;
	else if (modeString == "mania" || modeString == "osu!mania")
		return EGamemode::Mania;
	else
		throw Exception{SRC_POS, StrFormat("Invalid mode '{0}'", modeString)};
}

PP_NAMESPACE_END
