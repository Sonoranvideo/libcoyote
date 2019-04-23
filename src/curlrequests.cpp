/*
   Copyright 2019 Sonoran Video Systems

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "common.h"
#include "curlrequests.h"
#include <curl/curl.h>

typedef std::unique_ptr<curl_slist, std::function<decltype(curl_slist_free_all)> > CurlHeaderPtr;

bool CurlRequests::CurlSession::InitPerformed;

size_t CurlRequests::CurlSession::CurlWriteFunc(const char *Ptr, size_t UnitSize, size_t NumUnits, std::vector<uint8_t> *Response)
{
	const size_t DataSize = UnitSize * NumUnits;
	const size_t OldSize = Response->size();
	
	Response->resize(Response->size() + DataSize);
	
	memcpy(Response->data() + OldSize, Ptr, DataSize);
	
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

bool CurlRequests::CurlSession::SendJSON(const std::string &URL, const std::string &JSONData, std::vector<uint8_t> *Response)
{
	CURL *Desc = this->CurlDescriptor;
	
	curl_slist *Headers = curl_slist_append(nullptr, "Content-Type: application/vnd.api+json");
	Headers = curl_slist_append(Headers, "Connection: Keep-Alive");
	
	CurlHeaderPtr HeaderList { Headers, curl_slist_free_all };
	
	curl_easy_setopt(Desc, CURLOPT_URL, URL.c_str());
#ifdef DEBUG
	curl_easy_setopt(Desc, CURLOPT_VERBOSE, 1L);
#endif
	curl_easy_setopt(Desc, CURLOPT_HTTPHEADER, HeaderList.get());
	curl_easy_setopt(Desc, CURLOPT_WRITEFUNCTION, Response ? this->CurlWriteFunc : nullptr);
	curl_easy_setopt(Desc, CURLOPT_WRITEDATA, Response);
	curl_easy_setopt(Desc, CURLOPT_POST, 1L);
	curl_easy_setopt(Desc, CURLOPT_POSTFIELDSIZE, JSONData.length());
	curl_easy_setopt(Desc, CURLOPT_COPYPOSTFIELDS, JSONData.c_str());
	
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

#ifdef TESTING //Because it saves me, the developer, trouble.
int main(void)
{
	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);
	CurlRequests::CurlSession Session;
	
	std::vector<uint8_t> Response;
	Session.SendJSON("http://127.0.0.1:8000", "{ \"CoyoteAPIVersion\" : 0.1, \"CommandName\" : \"GetPresets\" }", &Response);
	
	puts((char*)Response.data());
	
	return 0;
}
#endif //TESTING
