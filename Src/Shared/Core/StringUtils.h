#pragma once

#include <chrono>
#include <string>

template <typename T>
static std::string durationToString(T dur)
{
	using namespace std::chrono;
	using day_t = duration<long long, std::ratio<3600 * 24>>;

	auto d = duration_cast<day_t>(dur);
	auto h = duration_cast<hours>(dur -= d);
	auto m = duration_cast<minutes>(dur -= h);
	auto s = duration_cast<seconds>(dur -= m);

	std::string result = StrFormat("{0w2arf0}s", s.count());
	if (m.count() > 0)
		result = StrFormat("{0w2arf0}m", m.count()) + result;
	if (h.count() > 0)
		result = StrFormat("{0w2arf0}h", h.count()) + result;
	if (d.count() > 0)
		result = StrFormat("{0}d", d.count()) + result;

	if (result[0] == '0')
		result.erase(result.begin());

	return result;
}
