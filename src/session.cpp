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
#define MAPARG(x) { #x, x }
struct InternalSession
{
	CurlRequests::CurlSession CurlObj;
	std::string Server;
	uint16_t PortNum;
	
	const std::string GetURL(void) const;
	Json::Value PerformJsonAction(const std::string &CommandName, Coyote::StatusCode *StatusOut = nullptr, const std::map<std::string, Json::Value> *Values = nullptr);

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

Json::Value InternalSession::PerformJsonAction(const std::string &CommandName, Coyote::StatusCode *StatusOut, const std::map<std::string, Json::Value> *Values)
{	
	Json::Value Msg { JsonProc::CreateJsonMsg(CommandName, Values) };
	
	
	std::vector<uint8_t> Response{};

	if (!this->CurlObj.SendJSON(this->GetURL(), Msg.toStyledString(), &Response))
	{
		return Coyote::STATUS_NETWORKERROR;
	}
	
	Json::Value &&IncomingMsg = JsonProc::ProcessJsonMsg((const char*)Response.data());
	
	if (StatusOut)
	{
		*StatusOut = JsonProc::GetStatusCode(IncomingMsg);
	}
	
	return IncomingMsg;
}

Coyote::StatusCode Coyote::Session::Take(const int32_t PK)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(PK) };
	
	SESS.PerformJsonAction("Take", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::Pause(const int32_t PK)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(PK) };
	
	SESS.PerformJsonAction("Pause", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::End(const int32_t PK)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(PK) };
	
	SESS.PerformJsonAction("End", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeleteAsset(const std::string &AssetName)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(AssetName) };
	
	SESS.PerformJsonAction("DeleteAsset", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::InstallAsset(const std::string &AssetPath)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(AssetPath) };
	
	SESS.PerformJsonAction("InstallAsset", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RenameAsset(const std::string &CurrentName, const std::string &NewName)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(CurrentName), MAPARG(NewName) };
	
	SESS.PerformJsonAction("RenameAsset", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SeekTo(const int32_t PK, const uint32_t TimeIndex)
{
	DEF_SESS;
	
	const std::map<std::string, Json::Value> Values { MAPARG(PK), MAPARG(TimeIndex) };

	Coyote::StatusCode Status{};

	SESS.PerformJsonAction("SeekTo", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetTimeCode(Coyote::TimeCode &Out)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const Json::Value &Msg = SESS.PerformJsonAction("GetTimeCode", &Status);
	
	if (Status != Coyote::STATUS_OK) return Status;
	
	const Json::Value &Data = JsonProc::GetDataField(Msg);
	
	assert(Data.isObject());
	
	std::unique_ptr<Coyote::TimeCode> TimeCodePtr { JsonProc::JSONToCoyoteTimeCode(Data) };
	Out = std::move(*TimeCodePtr); //Moving is not actually useful for a struct full of POD data types, but it's the thought that counts.
	
	return Status;
}
	
Coyote::StatusCode Coyote::Session::GetAssets(std::vector<Coyote::Asset> &Out)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const Json::Value &Msg = SESS.PerformJsonAction("GetAssets", &Status);
	
	if (Status != Coyote::STATUS_OK) return Status;
	
	const Json::Value &Data = JsonProc::GetDataField(Msg);
	
	assert(Data.isArray());
	
	Out.reserve(Data.size());
	
	for (auto Iter = Data.begin(); Iter != Data.end(); ++Iter)
	{
		std::unique_ptr<Coyote::Asset> CurAsset { JsonProc::JSONToCoyoteAsset(*Iter) };
		Out.push_back(std::move(*CurAsset));
	}

	return Status;
}

Coyote::StatusCode Coyote::Session::GetPresets(std::vector<Coyote::Preset> &Out)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const Json::Value &Msg = SESS.PerformJsonAction("GetPresets", &Status);
	
	if (Status != Coyote::STATUS_OK) return Status;
	
	const Json::Value &Data = JsonProc::GetDataField(Msg);
	
	assert(Data.isArray());
	
	Out.reserve(Data.size());
	
	for (auto Iter = Data.begin(); Iter != Data.end(); ++Iter)
	{
		std::unique_ptr<Coyote::Preset> CurPreset { JsonProc::JSONToCoyotePreset(*Iter) };
		Out.push_back(std::move(*CurPreset));
	}
	
	return Status;
}


#ifdef TESTING_SESSION
int main(void)
{
	std::vector<Coyote::Preset> Results;
	
	Coyote::Session SessObj("localhost");
	
	if (SessObj.GetPresets(Results) == Coyote::STATUS_OK)
	{
		std::cout << "Worked" << std::endl;
	}
	else
	{
		std::cout << "Failed" << std::endl;
	}
	
	for (size_t Inc = 0u, Size = Results.size(); Inc < Size; ++Inc)
	{
		std::cout << Results[Inc].Name << std::endl;
	}
	
}
#endif
