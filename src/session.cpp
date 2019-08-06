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
#include "../wsmessages/wsmessages.hpp"
#include "common.h"
#include "native_ws.h"
#include "asynctosync.h"
#include "asyncmsgs.h"
#include "msgpackproc.h"
#include "subscriptions.h"
#include "include/statuscodes.h"
#include "include/datastructures.h"
#include "include/session.h"
#include <mutex>

#define DEF_SESS InternalSession &SESS = *static_cast<InternalSession*>(this->Internal)
#define MAPARG(x) { #x, msgpack::object{ x, MsgpackProc::Zone } }


struct InternalSession
{
	AsyncToSync::SynchronousSession SyncSess;
	AsyncMsgs::AsynchronousSession ASyncSess;
	WS::WSConnection *Connection;
	std::string Host;
	time_t TimeoutSecs;
	
	const std::map<std::string, msgpack::object> PerformSyncedCommand(const std::string &CommandName, Coyote::StatusCode *StatusOut = nullptr, const msgpack::object *Values = nullptr);
	Coyote::StatusCode CreatePreset_Multi(const Coyote::Preset &Ref, const std::string &Cmd);
	
	static bool OnMessageReady(WS::WSConnection *Conn, WSMessage *Msg);
	bool CheckWSInit(void);
	
	inline bool ConfigConnection(void)
	{
		this->CheckWSInit();
		delete this->Connection;
		
		this->Connection = WS::WSCore::GetInstance()->NewConnection(this->Host, this);
		
		this->PerformSyncedCommand("SubscribeTC");
		
		return true;
	}
	
	inline InternalSession(const std::string &Host = "")
		: Connection(),
		Host(Host),
		TimeoutSecs(Coyote::Session::DefaultCommandTimeoutSecs) //10 second default operation timeout
	{
		this->ConfigConnection();
	}
	inline ~InternalSession(void) { delete this->Connection; }
};



bool InternalSession::CheckWSInit(void)
{
	static std::atomic_bool Entered;

	if (!Entered)
	{
		WS::WSCore::Fireup(&InternalSession::OnMessageReady);
		Entered = true;
		return false;
	}
	return true;
}

bool InternalSession::OnMessageReady(WS::WSConnection *Conn, WSMessage *Msg)
{
	InternalSession *Sess = static_cast<InternalSession*>(Conn->UserData);
	
	
	std::map<std::string, msgpack::object> Values;
	
	try
	{
		Values = MsgpackProc::InitIncomingMsg(Msg->GetBody(), Msg->GetBodySize());
	}
	catch(...)
	{
		return false;
	}
	
	const bool IsSynchronousMsg = Values.count("MsgID");
	
	//const bool Success = IsSynchronousMsg ? Sess->SyncSess.OnMessageReady(Values, Sess->Connection, Msg) : false;
	const bool Success = IsSynchronousMsg ? Sess->SyncSess.OnMessageReady(Values, Sess->Connection, Msg) : Sess->ASyncSess.OnMessageReady(Values, Sess->Connection, Msg);
	
	return Success;
}


const std::map<std::string, msgpack::object> InternalSession::PerformSyncedCommand(const std::string &CommandName, Coyote::StatusCode *StatusOut, const msgpack::object *Values)
{
	msgpack::sbuffer Buffer;
	msgpack::packer<msgpack::sbuffer> Pack { Buffer };
	
	//Acquire a new message ID
	const uint64_t MsgID = this->SyncSess.NewMsgID();
	
	//Pack our values into a msgpack buffer
	MsgpackProc::InitOutgoingMsg(Pack, CommandName, MsgID, Values);
	
	if (this->Connection->HasError())
	{
		//Try to reconnect
		this->ConfigConnection();
		
		return PerformSyncedCommand(CommandName, StatusOut, Values);
	}
	
	this->Connection->Send(new WSMessage(Buffer.data(), Buffer.size()));
	
	//Wait for the value we want (with the message ID we want) to appear in the WebSockets thread.
	AsyncToSync::MessageTicket *Ticket = this->SyncSess.NewTicket(MsgID);
	
	
	std::unique_ptr<WSMessage> ResponsePtr { Ticket->WaitForRecv(this->TimeoutSecs) };
	this->SyncSess.DestroyTicket(Ticket);
	
	if (!ResponsePtr)
	{
		if (StatusOut) *StatusOut = Coyote::COYOTE_STATUS_NETWORKERROR;

		return {};
	}
	
	//Decode the messagepack "map" into a real map of other messagepack objects.
	const std::map<std::string, msgpack::object> Results { MsgpackProc::InitIncomingMsg(ResponsePtr->GetBody(), ResponsePtr->GetBodySize()) };
	
	//Get the status code.
	assert(Results.count("StatusInt"));
	
	if (StatusOut) *StatusOut = static_cast<Coyote::StatusCode>(Results.at("StatusInt").as<int>());
	
	return Results;
}

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
	{ Coyote::COYOTE_RES_1080I, "1080i" },
};

Coyote::Session::Session(const std::string &Host) : Internal(new InternalSession{Host})
{
	InternalSession &Sess = *static_cast<InternalSession*>(this->Internal);
	
	(void)&Sess; //stfu gcc
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

Coyote::StatusCode Coyote::Session::Take(const int32_t PK)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { MAPARG(PK) };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("Take", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::TakeNext(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("TakeNext", &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::TakePrev(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("TakePrev", &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::Pause(const int32_t PK)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { MAPARG(PK) };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("Pause", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::End(const int32_t PK)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { MAPARG(PK) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("End", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeleteAsset(const std::string &AssetName)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "AssetName", msgpack::object{AssetName.c_str(), MsgpackProc::Zone } } };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	SESS.PerformSyncedCommand("DeleteAsset", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::InstallAsset(const std::string &AssetPath)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "AssetPath",  msgpack::object{AssetPath.c_str(), MsgpackProc::Zone } } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("InstallAsset", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RenameAsset(const std::string &CurrentName, const std::string &NewName)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "CurrentName", msgpack::object{CurrentName.c_str(), MsgpackProc::Zone } }, { "NewName", msgpack::object{NewName.c_str(), MsgpackProc::Zone} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("RenameAsset", &Status, &Pass);
	
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

	const msgpack::object &Data = MsgpackProc::PackCoyoteObject(&Ref);

	this->PerformSyncedCommand(Cmd, &Status, &Data);
	
	return Status;
}
	
Coyote::StatusCode Coyote::Session::BeginUpdate(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("BeginUpdate", &Status);
	
	return Status;
}


Coyote::StatusCode Coyote::Session::RebootCoyote(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("RebootCoyote", &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SoftRebootCoyote(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("SoftRebootCoyote", &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ShutdownCoyote(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("ShutdownCoyote", &Status);
	
	return Status;
}

	
Coyote::StatusCode Coyote::Session::IsUpdateDetected(bool &ValueOut)
{
	DEF_SESS;
	
	const char *CmdName = "IsUpdateDetected";
	
	Coyote::StatusCode Status{};

	const std::map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand(CmdName, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;	
	
	std::map<std::string, msgpack::object> DataField;
	
	Response.at("Data").convert(DataField);
		
	ValueOut = DataField[CmdName].as<int>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetDisks(std::vector<std::string> &Out)
{
	DEF_SESS;
	
	const char *CmdName = "GetDisks";
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand(CmdName, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	const msgpack::object &Results = Response.at("Data");
	
	//Convert into the array they are.
	std::vector<msgpack::object> Disks;
	Results.convert(Disks);
	
	//Each is a map of disk properties. Right now just drive letters.
	Out.clear();
	Out.reserve(Disks.size());

	for (msgpack::object &Item : Disks)
	{
		std::map<std::string, std::string> Map;
		Item.convert(Map);
		
		Out.push_back(Map.at("DriveLetter"));
	}
	
	return Status;
}

Coyote::StatusCode Coyote::Session::EjectDisk(const std::string &DriveLetter)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "DriveLetter", msgpack::object{DriveLetter.c_str()} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	const std::map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand("EjectDisk", &Status, &Pass) };
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ReorderPresets(const int32_t PK1, const int32_t PK2)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	const std::map<std::string, msgpack::object> Values { MAPARG(PK1), MAPARG(PK2) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	SESS.PerformSyncedCommand("ReorderPresets", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RestartService(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("RestartService", &Status);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::DeletePreset(const int32_t PK)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	const std::map<std::string, msgpack::object> Values { MAPARG(PK) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	SESS.PerformSyncedCommand("DeletePreset", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SeekTo(const int32_t PK, const uint32_t TimeIndex)
{
	DEF_SESS;
	
	const std::map<std::string, msgpack::object> Values { MAPARG(PK), MAPARG(TimeIndex) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	Coyote::StatusCode Status{};

	SESS.PerformSyncedCommand("SeekTo", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetTimeCode(Coyote::TimeCode &Out, const int32_t Timeout)
{
	DEF_SESS;

	if (Coyote::TimeCode *const TC = SESS.ASyncSess.SubSession.GetTimeCode())
	{
		Out = *TC;
		delete TC;
		return Coyote::COYOTE_STATUS_OK;
	}
	
	return Coyote::COYOTE_STATUS_FAILED;
}
	
Coyote::StatusCode Coyote::Session::GetAssets(std::vector<Coyote::Asset> &Out, const int32_t Timeout)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { MAPARG(Timeout) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetAssets", &Status, Timeout > 0 ? &Pass : nullptr) };
	
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::vector<msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	Out.reserve(Data.size());
	
	for (auto Iter = Data.begin(); Iter != Data.end(); ++Iter)
	{
		std::unique_ptr<Coyote::Asset> CurAsset { static_cast<Coyote::Asset*>(MsgpackProc::UnpackCoyoteObject(*Iter, typeid(Coyote::Asset)) ) };
		Out.push_back(std::move(*CurAsset));
	}

	return Status;
}

Coyote::StatusCode Coyote::Session::GetPresets(std::vector<Coyote::Preset> &Out, const int32_t Timeout)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { MAPARG(Timeout) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetPresets", &Status, Timeout > 0 ? &Pass : nullptr) };
	
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::vector<msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	Out.reserve(Data.size());
	
	for (auto Iter = Data.begin(); Iter != Data.end(); ++Iter)
	{
		std::unique_ptr<Coyote::Preset> CurPreset { static_cast<Coyote::Preset*>(MsgpackProc::UnpackCoyoteObject(*Iter, typeid(Coyote::Preset)) ) };
		Out.push_back(std::move(*CurPreset));
	}

	return Status;
}

Coyote::StatusCode Coyote::Session::GetHardwareState(Coyote::HardwareState &Out)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Msg { SESS.PerformSyncedCommand("GetHardwareState", &Status) };

	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unique_ptr<Coyote::HardwareState> Ptr { static_cast<Coyote::HardwareState*>(MsgpackProc::UnpackCoyoteObject(Msg.at("Data"), typeid(Coyote::HardwareState))) };
	
	Out = std::move(*Ptr);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetIP(const int32_t AdapterID, Coyote::NetworkInfo &Out)
{
	DEF_SESS;
	
	const std::map<std::string, msgpack::object> Values { MAPARG(AdapterID) };
	
	Coyote::StatusCode Status{};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetIP", &Status, &Pass) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unique_ptr<Coyote::NetworkInfo> Ptr { static_cast<Coyote::NetworkInfo*>(MsgpackProc::UnpackCoyoteObject(Msg.at("Data"), typeid(Coyote::NetworkInfo))) };
	
	Out = std::move(*Ptr);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetIP(const Coyote::NetworkInfo &Input)
{
	DEF_SESS;
	
	const std::map<std::string, msgpack::object> Values { { "AdapterID", msgpack::object{Input.AdapterID} }, { "Subnet", msgpack::object{Input.Subnet.GetCString()} }, { "IP", msgpack::object{Input.IP.GetCString()} } };
	
	Coyote::StatusCode Status{};

	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	SESS.PerformSyncedCommand("SetIP", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SelectPreset(const int32_t PK)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { MAPARG(PK) };

	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	SESS.PerformSyncedCommand("SelectPreset", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetMediaState(Coyote::MediaState &Out)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetMediaState", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	
	std::unique_ptr<Coyote::MediaState> Ptr { static_cast<Coyote::MediaState*>(MsgpackProc::UnpackCoyoteObject(Msg.at("Data"), typeid(Coyote::MediaState))) };
	
	Out = *Ptr;
	
	return Status;
}
	
Coyote::StatusCode Coyote::Session::InitializeCoyote(const Coyote::ResolutionMode Resolution, const Coyote::RefreshMode RefreshRate)
{
	DEF_SESS;
	
	assert(RefreshMap.count(RefreshRate));
	assert(ResolutionMap.count(Resolution));
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "Resolution", msgpack::object{ResolutionMap.at(Resolution).c_str()} }, { "Refresh", msgpack::object{RefreshMap.at(RefreshRate).c_str()} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	SESS.PerformSyncedCommand("InitializeCoyote", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetHardwareMode(const Coyote::ResolutionMode Resolution, const Coyote::RefreshMode RefreshRate)
{
	DEF_SESS;
	
	assert(RefreshMap.count(RefreshRate));
	assert(ResolutionMap.count(Resolution));
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "Resolution", msgpack::object{ResolutionMap.at(Resolution).c_str()} }, { "Refresh", msgpack::object{RefreshMap.at(RefreshRate).c_str()} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	SESS.PerformSyncedCommand("SetHardwareMode", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetServerVersion(std::string &Out)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetServerVersion", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("Version"));
	
	Out = Data.at("Version").as<std::string>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DetectUpdate(bool &DetectedOut, std::string *Out)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("DetectUpdate", &Status) };

	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	DetectedOut = Data.at("IsUpdateDetected").as<int>();
	
	if (Out) *Out = Data.at("Version").as<std::string>();
	
	return Status;
}

time_t Coyote::Session::GetCommandTimeoutSecs(void) const
{
	DEF_SESS;
	
	return SESS.TimeoutSecs;
}

void Coyote::Session::SetCommandTimeoutSecs(const time_t TimeoutSecs)
{
	DEF_SESS;
	
	SESS.TimeoutSecs = TimeoutSecs;
}

