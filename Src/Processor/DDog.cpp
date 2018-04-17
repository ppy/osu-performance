#include <PrecompiledHeader.h>

#include "DDog.h"

#ifndef __WIN32
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <stdio.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <unistd.h>
#endif

CDDog::CDDog(std::string host, s16 port)
: _host{host}, _port{port}
{
	memset((char*)&_server, 0, sizeof(_server));

#ifdef __WIN32
	_server.sin_family = AF_INET;
	_server.sin_port = htons(_port);
	_server.sin_addr.S_un.S_addr = inet_addr(_host.c_str());
#else
	_server.sin_family = AF_INET;
	_server.sin_port = htons(_port);
	inet_aton(_host.c_str(), &_server.sin_addr);
#endif
}

void CDDog::Increment(std::string metric, s64 amount, std::vector<std::string> tags, f32 sampleRate)
{
	UpdateStats(metric, amount, tags, sampleRate);
}

void CDDog::Decrement(std::string metric, s64 amount, std::vector<std::string> tags, f32 sampleRate)
{
	UpdateStats(metric, -amount, tags, sampleRate);
}

void CDDog::Timing(std::string metric, s64 value, std::vector<std::string> tags, f32 sampleRate)
{
	Send(StrFormat("{0}:{1}|ms", metric, value), tags, sampleRate);
}

void CDDog::Gauge(std::string metric, s64 value, std::vector<std::string> tags, f32 sampleRate)
{
	Send(StrFormat("{0}:{1}|g", metric, value), tags, sampleRate);
}

void CDDog::Histogram(std::string metric, s64 value, std::vector<std::string> tags, f32 sampleRate)
{
	Send(StrFormat("{0}:{1}|h", metric, value), tags, sampleRate);
}

void CDDog::Set(std::string metric, s64 value, std::vector<std::string> tags, f32 sampleRate)
{
	Send(StrFormat("{0}:{1}|s", metric, value), tags, sampleRate);
}

void CDDog::AddTags(std::string& message, const std::vector<std::string>& tags)
{
	if(tags.empty())
		return;

	message.push_back('|');

	for(const std::string& s : tags)
		message += StrFormat("#{0},", s);

	message.pop_back();
}


void CDDog::UpdateStats(std::string metric, s64 delta, const std::vector<std::string>& tags, f32 sampleRate)
{
	if (delta == 0)
		return;

	Send(StrFormat("{0}:{1}|c", metric, delta), tags, sampleRate);
}

void CDDog::Send(std::string data, const std::vector<std::string>& tags, f32 sampleRate)
{
	static thread_local std::mt19937_64 generator{
		std::hash<std::thread::id>()(std::this_thread::get_id())
	};
	static thread_local std::uniform_real_distribution<f32> distribution{0.0f, 1.0f};

	if(sampleRate < 1.0f)
	{
		if (distribution(generator) >= sampleRate)
			return;

		data += StrFormat("|@{0p2}", sampleRate);
	}

	AddTags(data, tags);

#ifdef __WIN32
	SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s == INVALID_SOCKET)
	{
		Log(CLog::Error, "Couldn't create UDP socket for datadog.");
		return;
	}

	// nonblocking
	u_long mode = 1;
	ioctlsocket(s, FIONBIO, &mode);

	s32 bytesSent = sendto(s, data.c_str(), (s32)data.length(), 0, (sockaddr*)&_server, sizeof(_server));
	if(bytesSent == SOCKET_ERROR)
		Log(CLog::Error, "Couldn't send data to datadog.");

	closesocket(s);
#else
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s == -1)
	{
		Log(CLog::Error, "Couldn't create UDP socket for datadog.");
		return;
	}

	// nonblocking
	int flags = fcntl(s, F_GETFL, 0);
	fcntl(s, F_SETFL, flags | O_NONBLOCK);

	s32 bytesSent = sendto(s, data.c_str(), data.length(), 0, (sockaddr*)&_server, sizeof(_server));
	if(bytesSent == -1)
		Log(CLog::Error, "Couldn't send data to datadog.");

	close(s);
#endif
}
