#pragma once

class DDog
{
public:
	DDog(std::string host, s16 port);

	void Increment(std::string metric, s64 amount = 1, std::vector<std::string> tags = {}, f32 sampleRate = 1);
	void Decrement(std::string metric, s64 amount = 1, std::vector<std::string> tags = {}, f32 sampleRate = 1);
	void Timing(std::string metric, s64 value, std::vector<std::string> tags = {}, f32 sampleRate = 1);
	void Gauge(std::string metric, s64 value, std::vector<std::string> tags = {}, f32 sampleRate = 1);
	void Histogram(std::string metric, s64 value, std::vector<std::string> tags = {}, f32 sampleRate = 1);
	void Set(std::string metric, s64 value, std::vector<std::string> tags = {}, f32 sampleRate = 1);

private:
	void addTags(std::string& message, const std::vector<std::string>& tags);

	void updateStats(std::string metric, s64 delta, const std::vector<std::string>& tags, f32 sampleRate);
	void send(std::string data, const std::vector<std::string>& tags, f32 sampleRate);

	std::string _host;
	s16 _port;

	sockaddr_in _server;
};
