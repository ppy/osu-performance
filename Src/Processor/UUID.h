#pragma once

class CUUID
{
public:
	static CUUID V4();

public:
	std::string ToString();
	uint8_t bytes[16];
};
