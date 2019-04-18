#include "common.h"
#include "curlrequests.h"
#include <curl/curl.h>
#include <functional>

typedef std::unique_ptr<curl_slist, std::function<decltype(curl_slist_free_all)> > CurlHeaderPtr;

size_t CurlRequests::CurlSession::CurlWriteFunc(char *Ptr, size_t UnitSize, size_t NumUnits, std::vector<uint8_t> *Response)
{
	const size_t DataSize = UnitSize * NumUnits;
	
	Response->resize(Response->size() + DataSize);
	
	memcpy(Response->data() + Response->size(), Ptr, DataSize);
	
	return DataSize;
}
	
CurlRequests::CurlSession::CurlSession(void)
{
	if (!this->InitPerformed)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		this->InitPerformed = true;
	}
	
	this->CurlDescriptor = curl_easy_init();
	
}

CurlRequests::CurlSession::~CurlSession(void)
{
	curl_easy_cleanup(static_cast<CURL*>(this->CurlDescriptor));
}

bool CurlRequests::CurlSession::SendJSON(const char *URL, const char *JSONData, std::vector<uint8_t> *Response)
{
	CURL *Desc = this->CurlDescriptor;
	
	CurlHeaderPtr HeaderList { curl_slist_append(nullptr, "Content-Type: application/vnd.api+json"), curl_slist_free_all };
	
	curl_easy_setopt(Desc, CURLOPT_URL, URL);
	curl_easy_setopt(Desc, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(Desc, CURLOPT_HTTPHEADER, HeaderList.get());
	curl_easy_setopt(Desc, CURLOPT_WRITEFUNCTION, Response ? this->CurlWriteFunc : nullptr);
	curl_easy_setopt(Desc, CURLOPT_WRITEDATA, Response);
	curl_easy_setopt(Desc, CURLOPT_POST, 1L);
	curl_easy_setopt(Desc, CURLOPT_POSTFIELDSIZE, strlen(JSONData));
	curl_easy_setopt(Desc, CURLOPT_COPYPOSTFIELDS, JSONData);
	
	Response->reserve(8192);
	
	int8_t AttemptsRemaining = 3;
	
	CURLcode Code{};
	do
	{
		Response->clear();
			
		Code = curl_easy_perform(Desc);
		
	} while (--AttemptsRemaining > 0 && Code != CURLE_OK);
	
	
	if (Code == CURLE_OK)
	{
		return true;
	}
	
	
	return false;
}
