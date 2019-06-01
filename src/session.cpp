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

#include "include/internal/common.h"
#include "include/internal/curlrequests.h"
#include "include/internal/jsonproc.h"
#include "include/statuscodes.h"
#include "include/datastructures.h"
#include "include/session.h"
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
	Coyote::StatusCode CreatePreset_Multi(const Coyote::Preset &Ref, const std::string &Cmd);

};


static const std::map<Coyote::RefreshMode, std::string> RefreshMap
{
	{ Coyote::COYOTE_REFRESH_23_98, "23.98" },
	{ Coyote::COYOTE_REFRESH_24, "24" },
	{ Coyote::COYOTE_REFRESH_25, "25" },
	{ Coyote::COYOTE_REFRESH_29_97, "29.97" },
	{ Coyote::COYOTE_REFRESH_30, "30" },
	{ Coyote::COYOTE_REFRESH_50, "50" },
	{ Coyote::COYOTE_REFRESH_59_94, "59.94" },
	{ Coyote::COYOTE_REFRESH_60, "60" }
};

static const std::map<Coyote::ResolutionMode, std::string> ResolutionMap
{
	{ Coyote::COYOTE_RES_1080P, "1080p" },
	{ Coyote::COYOTE_RES_2160P, "2160p" },
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
		if (StatusOut) *StatusOut = Coyote::COYOTE_STATUS_NETWORKERROR;
		
		return {};
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


Coyote::StatusCode Coyote::Session::CreatePreset(const Coyote::Preset &Ref)
{
	DEF_SESS;
	return SESS.CreatePreset_Multi(Ref, "CreatePreset");
}
Coyote::StatusCode Coyote::Session::UpdatePreset(const Coyote::Preset &Ref)
{
	DEF_SESS;
	return SESS.CreatePreset_Multi(Ref, "UpdatePreset");
}
Coyote::StatusCode Coyote::Session::LoadPreset(const Coyote::Preset &Ref)
{
	DEF_SESS;
	return SESS.CreatePreset_Multi(Ref, "LoadPreset");
}

Coyote::StatusCode InternalSession::CreatePreset_Multi(const Coyote::Preset &Ref, const std::string &Cmd)
{
	Coyote::StatusCode Status{};

	const Json::Value &Converted = JsonProc::CoyotePresetToJSON(Ref);
	
	const std::vector<std::string> Keys { Converted.getMemberNames() };
	
	std::map<std::string, Json::Value> Values{};
	
	for (std::string Key : Keys) Values[Key] = Converted[Key];
	
	this->PerformJsonAction("CreatePreset", &Status, &Values);
	
	return Status;
}
	
Coyote::StatusCode Coyote::Session::BeginUpdate(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformJsonAction("BeginUpdate", &Status);
	
	return Status;
}


Coyote::StatusCode Coyote::Session::RebootCoyote(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformJsonAction("RebootCoyote", &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SoftRebootCoyote(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformJsonAction("SoftRebootCoyote", &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ShutdownCoyote(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformJsonAction("ShutdownCoyote", &Status);
	
	return Status;
}

	
Coyote::StatusCode Coyote::Session::IsUpdateDetected(bool &ValueOut)
{
	DEF_SESS;
	
	const char *CmdName = "IsUpdateDetected";
	
	Coyote::StatusCode Status{};

	const Json::Value &Response { SESS.PerformJsonAction(CmdName, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;	
	
	const Json::Value &Result = JsonProc::GetDataField(Response);
	
	assert(Result.isMember(CmdName) && Result[CmdName].isBool());
	
	ValueOut = Result[CmdName].asBool();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetDisks(std::vector<std::string> &Out)
{
	DEF_SESS;
	
	const char *CmdName = "GetDisks";
	
	Coyote::StatusCode Status{};
	
	const Json::Value &Response { SESS.PerformJsonAction(CmdName, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	const Json::Value &Results = JsonProc::GetDataField(Response, Json::Value{ Json::arrayValue } );
	
	assert(Results.isArray());
	
	Out.reserve(Results.size());
	Out.clear();
	
	for (const Json::Value &Object : Results)
	{
		assert(Object.isMember("DriveLetter"));
		
		Out.push_back(Object["DriveLetter"].asString());
		
	}
	
	return Status;
}

Coyote::StatusCode Coyote::Session::EjectDisk(const std::string &DriveLetter)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(DriveLetter) };
	
	const Json::Value &Response { SESS.PerformJsonAction("EjectDisk", &Status, &Values) };
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ReorderPresets(const int32_t PK1, const int32_t PK2)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	const std::map<std::string, Json::Value> Values { MAPARG(PK1), MAPARG(PK2) };
	
	SESS.PerformJsonAction("ReorderPresets", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RestartService(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformJsonAction("RestartService", &Status);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::DeletePreset(const int32_t PK)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	const std::map<std::string, Json::Value> Values { MAPARG(PK) };
	
	SESS.PerformJsonAction("DeletePreset", &Status, &Values);
	
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

Coyote::StatusCode Coyote::Session::GetTimeCode(Coyote::TimeCode &Out, const int32_t Timeout)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(Timeout) };

	const Json::Value &Msg = SESS.PerformJsonAction("GetTimeCode", &Status, Timeout > 0 ? &Values : nullptr);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	const Json::Value &Data = JsonProc::GetDataField(Msg, { Json::objectValue });
	
	assert(Data.isObject());
	
	std::unique_ptr<Coyote::TimeCode> TimeCodePtr { JsonProc::JSONToCoyoteTimeCode(Data) };
	Out = std::move(*TimeCodePtr); //Moving is not actually useful for a struct full of POD data types, but it's the thought that counts.
	
	return Status;
}
	
Coyote::StatusCode Coyote::Session::GetAssets(std::vector<Coyote::Asset> &Out, const int32_t Timeout)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(Timeout) };

	const Json::Value &Msg = SESS.PerformJsonAction("GetAssets", &Status, Timeout > 0 ? &Values : nullptr);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
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

Coyote::StatusCode Coyote::Session::GetPresets(std::vector<Coyote::Preset> &Out, const int32_t Timeout)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(Timeout) };

	const Json::Value &Msg = SESS.PerformJsonAction("GetPresets", &Status, Timeout > 0 ? &Values : nullptr);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
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

Coyote::StatusCode Coyote::Session::GetHardwareState(Coyote::HardwareState &Out)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const Json::Value &Msg = SESS.PerformJsonAction("GetHardwareState", &Status);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	const Json::Value &Data = JsonProc::GetDataField(Msg, { Json::objectValue });
	
	assert(Data.isObject());
	
	std::unique_ptr<Coyote::HardwareState> Ptr { JsonProc::JSONToCoyoteHardwareState(Data) };
	
	Out = std::move(*Ptr);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetIP(const int32_t AdapterID, Coyote::NetworkInfo &Out)
{
	DEF_SESS;
	
	const std::map<std::string, Json::Value> Values { MAPARG(AdapterID) };
	
	Coyote::StatusCode Status{};
	
	const Json::Value &Msg = SESS.PerformJsonAction("GetIP", &Status, &Values);
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	const Json::Value &Data = JsonProc::GetDataField(Msg, { Json::objectValue });
	
	assert(Data.isObject());
	
	std::unique_ptr<Coyote::NetworkInfo> Ptr { JsonProc::JSONToCoyoteNetworkInfo(Data) };
	
	Out = std::move(*Ptr);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetIP(const Coyote::NetworkInfo &Input)
{
	DEF_SESS;
	
	const std::map<std::string, Json::Value> Values { { "AdapterID", Input.AdapterID }, { "Subnet", Input.Subnet.GetStdString() }, { "IP", Input.IP.GetStdString() } };
	
	Coyote::StatusCode Status{};
	
	SESS.PerformJsonAction("SetIP", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SelectPreset(const int32_t PK)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { MAPARG(PK) };
	
	SESS.PerformJsonAction("SelectPreset", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetMediaState(Coyote::MediaState &Out)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const Json::Value &Msg { SESS.PerformJsonAction("GetMediaState", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	const Json::Value &Data { JsonProc::GetDataField(Msg) };
	
	std::unique_ptr<Coyote::MediaState> Ptr { JsonProc::JSONToCoyoteMediaState(Data) };
	
	Out = *Ptr;
	
	return Status;
}
	
Coyote::StatusCode Coyote::Session::SetHardwareMode(const Coyote::ResolutionMode Resolution, const Coyote::RefreshMode RefreshRate)
{
	DEF_SESS;
	
	assert(RefreshMap.count(RefreshRate));
	assert(ResolutionMap.count(Resolution));
	
	StatusCode Status{};
	
	const std::map<std::string, Json::Value> Values { { "Resolution", ResolutionMap.at(Resolution) }, { "Refresh", RefreshMap.at(RefreshRate) } };
	
	SESS.PerformJsonAction("SetHardwareMode", &Status, &Values);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetServerVersion(std::string &Out)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const Json::Value &Msg { SESS.PerformJsonAction("GetServerVersion", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	const Json::Value &Data { JsonProc::GetDataField(Msg) };
	
	Out = JsonProc::JSONToServerVersion(Data);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DetectUpdate(bool &DetectedOut, std::string *Out)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const Json::Value &Msg { SESS.PerformJsonAction("DetectUpdate", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	const Json::Value &Data { JsonProc::GetDataField(Msg) };
	
	auto Results = JsonProc::JSONToUpdateVersion(Data);
	
	DetectedOut = Results.second;
	if (Out) *Out = Results.first;
	
	return Status;
}
