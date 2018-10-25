#include <pp/Common.h>
#include <pp/performance/UUID.h>

PP_NAMESPACE_BEGIN

uint32_t rand32()
{
	// we need to do this, because there is no
	// gaurantee that RAND_MAX is >= 0xffffffff
	// in fact, it is LIKELY to be 0x7fffffff
	const uint32_t r1 = rand() & 0x0ff;
	const uint32_t r2 = rand() & 0xfff;
	const uint32_t r3 = rand() & 0xfff;

	return (r3 << 20) | (r2 << 8) | r1;
}

UUID UUID::V4()
{
	static const uint16_t c[] = {
		0x8000,
		0x9000,
		0xa000,
		0xb000,
	};

	UUID uuid;

	const uint32_t rand_1 = (rand32() & 0xffffffff);
	const uint32_t rand_2 = (rand32() & 0xffff0fff) | 0x4000;
	const uint32_t rand_3 = (rand32() & 0xffff0fff) | c[rand() & 0x03];
	const uint32_t rand_4 = (rand32() & 0xffffffff);

	uuid._bytes[0x00] = (rand_1 >> 24) & 0xff;
	uuid._bytes[0x01] = (rand_1 >> 16) & 0xff;
	uuid._bytes[0x02] = (rand_1 >> 8) & 0xff;
	uuid._bytes[0x03] = (rand_1)& 0xff;

	uuid._bytes[0x04] = (rand_2 >> 24) & 0xff;
	uuid._bytes[0x05] = (rand_2 >> 16) & 0xff;
	uuid._bytes[0x06] = (rand_2 >> 8) & 0xff;
	uuid._bytes[0x07] = (rand_2)& 0xff;

	uuid._bytes[0x08] = (rand_3 >> 24) & 0xff;
	uuid._bytes[0x09] = (rand_3 >> 16) & 0xff;
	uuid._bytes[0x0a] = (rand_3 >> 8) & 0xff;
	uuid._bytes[0x0b] = (rand_3)& 0xff;

	uuid._bytes[0x0c] = (rand_4 >> 24) & 0xff;
	uuid._bytes[0x0d] = (rand_4 >> 16) & 0xff;
	uuid._bytes[0x0e] = (rand_4 >> 8) & 0xff;
	uuid._bytes[0x0f] = (rand_4)& 0xff;

	return uuid;
}

std::string UUID::ToString()
{
	std::stringstream ss;
	ss << std::hex;

	for (unsigned int i = 0; i < 16; ++i)
		ss << std::setw(2) << std::setfill('0') << (int)_bytes[i];

	return ss.str();
}

PP_NAMESPACE_END
