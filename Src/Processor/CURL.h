#pragma once

typedef void CURL;

class CCURL
{
public:
	CCURL();
	~CCURL();

	void SendToSlack(
		std::string domain,
		std::string key,
		std::string username,
		std::string iconURL,
		std::string channel,
		std::string message
	);

	void SendToSentry(
		std::string domain,
		s32 projectID,
		std::string publicKey,
		std::string secretKey,
		class Exception& e,
		std::string mode,
		bool warning = false
	);

private:
	CURL* _pCURL = nullptr;
};
