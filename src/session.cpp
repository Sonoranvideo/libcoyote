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
#define WSMESSAGES_NOLWS
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

extern const std::map<Coyote::RefreshMode, std::string> RefreshMap
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

extern const std::map<Coyote::ResolutionMode, std::string> ResolutionMap
{
	{ Coyote::COYOTE_RES_1080P, "1080p" },
	{ Coyote::COYOTE_RES_2160P, "2160p" },
	{ Coyote::COYOTE_RES_1080I, "1080i" },
};

static const auto &ReverseRefreshMapObj { RebuildMapBackwards(RefreshMap) };
static const auto &ReverseResolutionMapObj { RebuildMapBackwards(ResolutionMap) };


Coyote::RefreshMode ReverseRefreshMap(const std::string &Lookup)
{
	if (!ReverseRefreshMapObj.count(Lookup)) return Coyote::COYOTE_REFRESH_INVALID;

	return ReverseRefreshMapObj.at(Lookup);
}

Coyote::ResolutionMode ReverseResolutionMap(const std::string &Lookup)
{
	if (!ReverseResolutionMapObj.count(Lookup)) return Coyote::COYOTE_RES_INVALID;
	
	return ReverseResolutionMapObj.at(Lookup);
}


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
		
		WS::WSCore *Core =  WS::WSCore::GetInstance();
		
		if (this->Connection) Core->ForgetConnection(this->Connection);
		this->Connection = Core->NewConnection(this->Host, this);
		
		this->SyncSess.DestroyAllTickets();

		this->PerformSyncedCommand("SubscribeTC");
		this->PerformSyncedCommand("SubscribeAssets");
		this->PerformSyncedCommand("SubscribePresets");
		this->PerformSyncedCommand("SubscribeHWState");
		
		return true;
	}
	
	inline InternalSession(const std::string &Host = "")
		: Connection(),
		Host(Host),
		TimeoutSecs(Coyote::Session::DefaultCommandTimeoutSecs) //10 second default operation timeout
	{
		this->ConfigConnection();
	}
	inline ~InternalSession(void)
	{
		WS::WSCore::GetInstance()->ForgetConnection(this->Connection);
	}
};



bool InternalSession::CheckWSInit(void)
{
	static std::mutex EnterLock;
	static std::atomic_bool Entered;
	
	const std::lock_guard<std::mutex> G { EnterLock };
	
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
		LDEBUG_MSG("Decoding WSMessage of size " << Msg->GetBodySize());
		Values = MsgpackProc::InitIncomingMsg(Msg->GetBody(), Msg->GetBodySize());
	}
	catch (const std::bad_cast &Error)
	{
		LDEBUG_MSG("Corrupted or malformed message received, exception was " << Error.what());
		return false;
	}
	catch (...)
	{
		LDEBUG_MSG("Unknown exception was caught decoding a message.");
		return false;
	}
	
	const bool IsSynchronousMsg = Values.count("MsgID");
	
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
	

	//Create the ticket BEFORE we send it.
	AsyncToSync::MessageTicket *Ticket = this->SyncSess.NewTicket(MsgID);

	this->Connection->Send(new WSMessage(Buffer.data(), Buffer.size()));
	
	//Wait for the value we want (with the message ID we want) to appear in the WebSockets thread.
		
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

Coyote::StatusCode Coyote::Session::SelectNext(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("SelectNext", &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SelectPrev(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("SelectPrev", &Status);
	
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

Coyote::StatusCode Coyote::Session::SetPausedState(const int32_t PK, const bool Value)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { MAPARG(PK) };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand(Value ? "SetPause" : "UnsetPause", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetPause(const int32_t PK)
{
	return this->SetPausedState(PK, true);
}

Coyote::StatusCode Coyote::Session::UnsetPause(const int32_t PK)
{
	return this->SetPausedState(PK, false);
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

Coyote::StatusCode Coyote::Session::RenameCountdown(const int32_t PK, const int32_t Time, const std::string &NewName)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, MsgpackProc::Zone} },
		{ "Time", msgpack::object{Time, MsgpackProc::Zone} },
		{ "NewName", msgpack::object{NewName.c_str(), MsgpackProc::Zone} }
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("RenameCountdown", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RenameGoto(const int32_t PK, const int32_t Time, const std::string &NewName)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, MsgpackProc::Zone} },
		{ "Time", msgpack::object{Time, MsgpackProc::Zone} },
		{ "NewName", msgpack::object{NewName.c_str(), MsgpackProc::Zone} }
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("RenameGoto", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeleteCountdown(const int32_t PK, const int32_t Time)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, MsgpackProc::Zone} },
		{ "Time", msgpack::object{Time, MsgpackProc::Zone} },
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("DeleteCountdown", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeleteGoto(const int32_t PK, const int32_t Time)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, MsgpackProc::Zone} },
		{ "Time", msgpack::object{Time, MsgpackProc::Zone} },
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("DeleteGoto", &Status, &Pass);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::CreateCountdown(const int32_t PK, const int32_t Time, const std::string &Name)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, MsgpackProc::Zone} },
		{ "Time", msgpack::object{Time, MsgpackProc::Zone} },
		{ "Name", msgpack::object{Name.c_str(), MsgpackProc::Zone} }
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("CreateCountdown", &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::CreateGoto(const int32_t PK, const int32_t Time, const std::string &Name)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, MsgpackProc::Zone} },
		{ "Time", msgpack::object{Time, MsgpackProc::Zone} },
		{ "Name", msgpack::object{Name.c_str(), MsgpackProc::Zone} }
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("CreateGoto", &Status, &Pass);
	
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

	
Coyote::StatusCode Coyote::Session::IsMirror(bool &ValueOut)
{
	DEF_SESS;
	
	const char *CmdName = "IsMirror";
	
	Coyote::StatusCode Status{};

	const std::map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand(CmdName, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::map<std::string, msgpack::object> DataField;
	
	Response.at("Data").convert(DataField);
		
	ValueOut = DataField[CmdName].as<int>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SynchronizerBusy(bool &ValueOut)
{
	DEF_SESS;
	
	const char *CmdName = "SynchronizerBusy";
	
	Coyote::StatusCode Status{};

	const std::map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand(CmdName, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;	
	
	std::map<std::string, msgpack::object> DataField;
	
	Response.at("Data").convert(DataField);
		
	ValueOut = DataField["Busy"].as<int>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeconfigureSync(void)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("DeconfigureSync", &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::AddMirror(const std::string &MirrorIP)
{
	DEF_SESS;
	
	Coyote::StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "BackupIP",  msgpack::object{MirrorIP.c_str(), MsgpackProc::Zone } } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	SESS.PerformSyncedCommand("AddMirror", &Status, &Pass);
	
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

Coyote::StatusCode Coyote::Session::GetTimeCode(Coyote::TimeCode &Out, const int32_t PK)
{
	DEF_SESS;

	if (Coyote::TimeCode *const TC = SESS.ASyncSess.SubSession.GetTimeCode(PK))
	{
		Out = *TC;
		delete TC;
		return Coyote::COYOTE_STATUS_OK;
	}
	
	return Coyote::COYOTE_STATUS_FAILED;
}
	
Coyote::StatusCode Coyote::Session::GetPresets(std::vector<Coyote::Preset> &Out)
{
	DEF_SESS;

	std::unique_ptr<std::map<int32_t, Coyote::Preset> > Ptr { SESS.ASyncSess.SubSession.GetPresets() };
	
	if (!Ptr) return Coyote::COYOTE_STATUS_FAILED;
	
	Out.clear();
	Out.reserve(Ptr->size());
	
	for (auto Pair : *Ptr)
	{
		Out.push_back(std::move(Pair.second));
	}

	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetAssets(std::vector<Coyote::Asset> &Out)
{
	DEF_SESS;

	std::unique_ptr<std::map<std::string, Coyote::Asset> > Ptr { SESS.ASyncSess.SubSession.GetAssets() };
	
	if (!Ptr) return Coyote::COYOTE_STATUS_FAILED;
	
	Out.clear();
	Out.reserve(Ptr->size());
	
	for (auto Pair : *Ptr)
	{
		Out.push_back(std::move(Pair.second));
	}

	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetHardwareState(Coyote::HardwareState &Out)
{
	DEF_SESS;
	
	std::unique_ptr<Coyote::HardwareState> Ptr { SESS.ASyncSess.SubSession.GetHardwareState() };
	
	if (!Ptr || Ptr->Resolution == Coyote::COYOTE_RES_INVALID) return Coyote::COYOTE_STATUS_FAILED;
	
	Out = std::move(*Ptr);
	
	return Coyote::COYOTE_STATUS_OK;
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

Coyote::StatusCode Coyote::Session::GetCurrentRole(Coyote::UnitRole &CurrentRoleOut)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetCurrentRole", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("CurrentRoleInt") && Data.count("CurrentRoleText"));
	
	CurrentRoleOut = static_cast<UnitRole>(Data.at("CurrentRoleInt").as<int>());
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetUnitID(std::string &UnitIDOut, std::string &NicknameOut)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetUnitID", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("UnitID") && Data.count("Nickname"));
	
	UnitIDOut = Data.at("UnitID").as<std::string>();
	NicknameOut = Data.at("Nickname").as<std::string>();
	
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

Coyote::StatusCode Coyote::Session::GetMirrors(std::vector<Coyote::Mirror> &Out)
{
	DEF_SESS;

	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetMirrors", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	
	std::vector<msgpack::object> Data;

	Msg.at("Data").convert(Data);
	
	for (msgpack::object &Object : Data)
	{
		LDEBUG_MSG("Found mirror data");
		std::unique_ptr<Coyote::Mirror> MirrorStruct { static_cast<Coyote::Mirror*>(MsgpackProc::UnpackCoyoteObject(Object, typeid(Coyote::Mirror))) };
		
		Out.emplace_back(std::move(*MirrorStruct));
	}

	return Coyote::COYOTE_STATUS_OK;
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

Coyote::StatusCode Coyote::Session::GetLogsZip(std::vector<uint8_t> &OutBuffer)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetLogsZip", &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::map<std::string, msgpack::object> Data;

	Msg.at("Data").convert(Data);
	
	Data.at("ZipBytes").convert(OutBuffer);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ReadLog(const std::string &SpokeName, const int Year, const int Month, const int Day, std::string &LogOut)
{
	DEF_SESS;
	
	StatusCode Status{};
	

	LDEBUG_MSG("Building values");
	const std::map<std::string, msgpack::object> Values { {"SpokeName", msgpack::object{SpokeName.c_str()} }, { "Year", msgpack::object{Year} }, { "Month", msgpack::object{Month} }, { "Day", msgpack::object{Day} } };
	
	LDEBUG_MSG("Performing conversions");
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };
	
	LDEBUG_MSG("Executing method");
	const std::map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("ReadLog", &Status, &Pass) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	
	std::map<std::string, msgpack::object> Data;

	LDEBUG_MSG("Converting Data");
	Msg.at("Data").convert(Data);
	
	LDEBUG_MSG("Converting LogText");
	Data.at("LogText").convert(LogOut);

	LDEBUG_MSG("Success");
	return Status;
}

Coyote::StatusCode Coyote::Session::ExportLogsZip(const std::string &DriveLetter)
{
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "DriveLetter", msgpack::object{DriveLetter.c_str()} } };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	SESS.PerformSyncedCommand("ExportLogsZip", &Status, &Pass);
	
	return Status;
}
	
Coyote::StatusCode Coyote::Session::_SVS_WriteCytLog_(const std::string &Param1, const std::string &Param2)
{ //This function is intended for Sonoran Video Systems use only. Use by users will either not work or just confuse unit logs.
	DEF_SESS;
	
	StatusCode Status{};
	
	const std::map<std::string, msgpack::object> Values { { "SpokeName", msgpack::object{Param1.c_str()} }, { "LogText", msgpack::object{Param2.c_str()} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values) };

	SESS.PerformSyncedCommand("__LOGWRITE__", &Status, &Pass);
	
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

void SessionSneak_DeactivateConnection(void *Ptr)
{
	InternalSession &SESS = *static_cast<InternalSession*>(Ptr);
	
	SESS.SyncSess.DestroyAllTickets();
}
