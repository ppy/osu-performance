#pragma once

#include <pp/common.h>

PP_NAMESPACE_BEGIN

class UUID
{
public:
	static UUID V4();

	std::string ToString();

private:
	uint8_t _bytes[16];
};

PP_NAMESPACE_END
