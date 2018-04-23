#pragma once

class CUUID
{
public:
	static CUUID V4();

	std::string ToString();

private:
	uint8_t _bytes[16];
};
