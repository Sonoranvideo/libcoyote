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
#include "statuscodes.h"
#include "datastructures.h"
#include "session.h"
#include "jsonproc.h"
#include <json/json.h>
#include <iostream>
#define DEF_SESS InternalSession &SESS = *static_cast<InternalSession*>(this->Internal)

struct InternalSession
{
	CurlRequests::CurlSession CurlObj;
	std::string Server;
	uint16_t PortNum;
	
	const std::string GetURL(void) const;
};

const std::string InternalSession::GetURL(void) const
{
	const char Http[] = "http://";
	
	std::string RetVal;
	
	if (strncmp(this->Server.c_str(), Http, sizeof Http) != 0)
	{
		RetVal += Http;
	}
	
	RetVal += this->Server + ':' + std::to_string(this->PortNum);
	
	return RetVal;
}


Coyote::Session::Session(const std::string &Server, const uint16_t PortNum) : Internal(new InternalSession{})
{
	InternalSession &Sess = *static_cast<InternalSession*>(this->Internal);
	
	Sess.Server = Server;
	Sess.PortNum = PortNum;
}

Coyote::Session::~Session(void)
{
	delete static_cast<InternalSession*>(this->Internal);
}

Coyote::Session::Session(Session &&In)
{
	this->Internal = In.Internal;
	In.Internal = nullptr;
}

Coyote::Session &Coyote::Session::operator=(Session &&In)
{
	if (this == &In) return *this;
	
	delete static_cast<InternalSession*>(this->Internal);
	
	this->Internal = In.Internal;
	In.Internal = nullptr;
	
	return *this;
}


Coyote::StatusCode Coyote::Session::GetAssets(std::vector<Coyote::Asset> &Out)
{
	DEF_SESS;
	
	Json::Value Msg { JsonProc::CreateJsonMsg("GetAssets") };
	
	CurlRequests::CurlSession Session{};
	
	std::vector<uint8_t> Response{};
	

	if (!Session.SendJSON(SESS.GetURL(), Msg.toStyledString(), &Response))
	{
		return Coyote::STATUS_NETWORKERROR;
	}
	
	
	return JsonProc::DecodeAssets((const char*)Response.data(), Out);
}

#ifdef TESTING_SESSION
int main(void)
{
	std::vector<Coyote::Asset> Results;
	
	Coyote::Session SessObj("localhost");
	
	if (SessObj.GetAssets(Results) == Coyote::STATUS_OK)
	{
		std::cout << "Worked" << std::endl;
	}
	else
	{
		std::cout << "Failed" << std::endl;
	}
	
	for (size_t Inc = 0u, Size = Results.size(); Inc < Size; ++Inc)
	{
		std::cout << Results[Inc] << std::endl;
	}
		
}
#endif
