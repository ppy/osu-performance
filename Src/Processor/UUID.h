#pragma once

class UUID
{
public:
	static UUID V4();

	std::string ToString();

private:
	uint8_t _bytes[16];
};
