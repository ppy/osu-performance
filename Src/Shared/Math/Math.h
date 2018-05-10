#pragma once

template <class T>
INLINE const T Clamp(const T& value, const T& low, const T& high)
{
	return std::min(std::max(value, low), high);
}
