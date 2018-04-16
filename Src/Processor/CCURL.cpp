#include <PrecompiledHeader.h>

#include "UUID.h"
#include "CCURL.h"

size_t EmptyCURLWriteData(void *buffer, size_t size, size_t nmemb, void *userp)
{
	return size * nmemb;
}

CCURL::CCURL()
{
	_pCURL = curl_easy_init();
	curl_easy_setopt(_pCURL, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(_pCURL, CURLOPT_WRITEFUNCTION, EmptyCURLWriteData);
}

CCURL::~CCURL()
{
	curl_easy_cleanup(_pCURL);
}

void CCURL::SendToSlack(
	std::string domain,
	std::string key,
	std::string username,
	std::string iconURL,
	std::string channel,
	std::string message)
{
	std::string url = StrFormat(
		"https://{0}/services/hooks/incoming-webhook?token={1}", domain, key);

	curl_easy_setopt(_pCURL, CURLOPT_URL, url.c_str());

	std::string PostData = StrFormat(R"(
			{{
				"channel":"{0}",
				"username":"{1}",
				"icon_url":"{2}",
				"text":"{3}"
			}
	)", channel, username, iconURL, message);

	curl_easy_setopt(_pCURL, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(_pCURL, CURLOPT_POSTFIELDS, PostData.c_str());

	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	std::string HTTPHeader = StrFormat("Content-Length: {0}", PostData.length());
	headers = curl_slist_append(headers, HTTPHeader.c_str());

	curl_easy_setopt(_pCURL, CURLOPT_HTTPHEADER, headers);

	CURLcode Error = curl_easy_perform(_pCURL);
	curl_slist_free_all(headers);

	if(Error != CURLE_OK)
	{
		Log(CLog::Error, StrFormat("Slack CURL error {0}", Error));
		return;
	}

	long ResponseCode;
	curl_easy_getinfo(_pCURL, CURLINFO_RESPONSE_CODE, &ResponseCode);

	if(ResponseCode != 200)
	{
		Log(CLog::Error, StrFormat("Slack CURL response {0}", ResponseCode));
		return;
	}

	Log(CLog::Success, StrFormat("Sent message to slack channel \"{0}\". \"{1}: {2}\".", channel, username, message));
}

void CCURL::SendToSentry(
	std::string domain,
	s32 projectID,
	std::string publicKey,
	std::string secretKey,
	CException& e,
	std::string mode,
	bool warning)
{
	std::string url = StrFormat("https://{0}/api/{1}/store/", domain, projectID);
	curl_easy_setopt(_pCURL, CURLOPT_URL, url.c_str());

	std::string file = e.File();
	std::replace(std::begin(file), std::end(file), '\\', '/');

	std::string postData = StrFormat(
	R"({{
		"event_id":"{0}",
		"message":"{1}",
		"level":"{2}",
		"tags": {{
			"mode":"{3}"
		},
		"extra": {{
			"file":"{4}",
			"line":"{5}"
		}
	})",
	CUUID::V4().ToString(),
	e.Description(),
	warning ? "warning" : "error",
	mode,
	file,
	e.Line());

	curl_easy_setopt(_pCURL, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(_pCURL, CURLOPT_POSTFIELDS, postData.c_str());

	struct curl_slist* headers = nullptr;
	std::string httpHeader = StrFormat(
		"X-Sentry-Auth: Sentry sentry_version=5,"
		"sentry_client=cpp/1.0,"
		"sentry_timestamp={0},"
		"sentry_key={1},"
		"sentry_secret={2}",
		std::time(nullptr), publicKey, secretKey);
	headers = curl_slist_append(headers, httpHeader.c_str());

	curl_easy_setopt(_pCURL, CURLOPT_HTTPHEADER, headers);

	CURLcode error = curl_easy_perform(_pCURL);
	curl_slist_free_all(headers);

	if(error != CURLE_OK)
	{
		Log(CLog::Error, StrFormat("Sentry CURL error {0}", error));
		return;
	}

	long responseCode;
	curl_easy_getinfo(_pCURL, CURLINFO_RESPONSE_CODE, &responseCode);

	if(responseCode != 200)
	{
		Log(CLog::Error, StrFormat("Sentry CURL response {0}", responseCode));
		return;
	}

	Log(CLog::Success, "Submitted exception to sentry.");
}
