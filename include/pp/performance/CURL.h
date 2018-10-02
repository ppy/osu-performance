#pragma once

#include <pp/Common.h>

#include <curl/curl.h>

typedef void CURL;

PP_NAMESPACE_BEGIN

class CURL
{
public:
	CURL();

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
		Exception& e,
		std::string mode,
		bool warning = false
	);

private:
	std::unique_ptr<::CURL, decltype(&curl_easy_cleanup)> _pCURL;
};

PP_NAMESPACE_END
