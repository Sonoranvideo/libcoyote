/*
   Copyright 2022 Sonoran Video Systems

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
#define MSGPACK_DEFAULT_API_VERSION 2
#include "msgpack.hpp"

#define WSMESSAGES_NOLWS
#include "../wsmessages/wsmessages.hpp"
#include "include/common.h"
#include "native_ws.h"
#include "asynctosync.h"
#include "asyncmsgs.h"
#include "msgpackproc.h"
#include "subscriptions.h"
#include "discovery.h"
#include "include/statuscodes.h"
#include "include/datastructures.h"
#include "include/session.h"
#include <mutex>

#define DEF_SESS InternalSession &SESS = *static_cast<InternalSession*>(this->Internal)
#define DEF_CONST_SESS const InternalSession &SESS = *static_cast<const InternalSession*>(this->Internal)
#define MAPARG(x) { #x, msgpack::object{ x, TempZone } }

extern EXPFUNC const std::unordered_map<Coyote::RefreshMode, std::string> Coyote::RefreshMap
{
	{ Coyote::COYOTE_REFRESH_INVALID, "" },
	{ Coyote::COYOTE_REFRESH_23_98, "23.98" },
	{ Coyote::COYOTE_REFRESH_24, "24" },
	{ Coyote::COYOTE_REFRESH_25, "25" },
	{ Coyote::COYOTE_REFRESH_29_97, "29.97" },
	{ Coyote::COYOTE_REFRESH_30, "30" },
	{ Coyote::COYOTE_REFRESH_50, "50" },
	{ Coyote::COYOTE_REFRESH_59_94, "59.94" },
	{ Coyote::COYOTE_REFRESH_60, "60" }
};

extern EXPFUNC const std::unordered_map<Coyote::ResolutionMode, std::string> Coyote::ResolutionMap
{
	{ Coyote::COYOTE_RES_INVALID, "" },
	{ Coyote::COYOTE_RES_720P, "720p" },
	{ Coyote::COYOTE_RES_1080I, "1080i" },
	{ Coyote::COYOTE_RES_1080P, "1080p" },
	{ Coyote::COYOTE_RES_1080PSF, "1080psf" },
	{ Coyote::COYOTE_RES_2160P, "2160p" },
	{ Coyote::COYOTE_RES_2160PSF, "2160psf" },
	{ Coyote::COYOTE_RES_DCI_2048, "DCI 2048" },
	{ Coyote::COYOTE_RES_DCI_2048PSF, "DCI 2048psf" },
	{ Coyote::COYOTE_RES_DCI_4096, "DCI 4096" },
	{ Coyote::COYOTE_RES_DCI_4096PSF, "DCI 4096psf" },
};

extern EXPFUNC const std::unordered_map<Coyote::ResolutionMode, Coyote::Size2D> Coyote::ResolutionSizeMap
{
	{ Coyote::COYOTE_RES_INVALID, {} },
	{ Coyote::COYOTE_RES_720P, { 1280, 720 } },
	{ Coyote::COYOTE_RES_1080P, { 1920, 1080 } },
	{ Coyote::COYOTE_RES_1080PSF, { 1920, 1080 } },
	{ Coyote::COYOTE_RES_2160P, { 3840, 2160 } },
	{ Coyote::COYOTE_RES_2160PSF, { 3840, 2160 } },
	{ Coyote::COYOTE_RES_DCI_4096, { 4096, 2160 } },
	{ Coyote::COYOTE_RES_DCI_4096PSF, { 4096, 2160 } },
	{ Coyote::COYOTE_RES_DCI_2048, { 2048, 1080 } },
	{ Coyote::COYOTE_RES_DCI_2048PSF, { 2048, 1080 } },
	{ Coyote::COYOTE_RES_1080I, { 1920, 1080 } },
};

static const auto &ReverseRefreshMapObj { RebuildMapBackwards(Coyote::RefreshMap) };
static const auto &ReverseResolutionMapObj { RebuildMapBackwards(Coyote::ResolutionMap) };


EXPFUNC Coyote::RefreshMode Coyote::ReverseRefreshMap(const std::string &Lookup)
{
	if (!ReverseRefreshMapObj.count(Lookup)) return Coyote::COYOTE_REFRESH_INVALID;

	return ReverseRefreshMapObj.at(Lookup);
}

EXPFUNC Coyote::ResolutionMode Coyote::ReverseResolutionMap(const std::string &Lookup)
{
	if (!ReverseResolutionMapObj.count(Lookup)) return Coyote::COYOTE_RES_INVALID;
	
	return ReverseResolutionMapObj.at(Lookup);
}

using Coyote::ResolutionMap;
using Coyote::RefreshMap;
using Coyote::ReverseResolutionMap;
using Coyote::ReverseRefreshMap;

struct InternalSession
{
	std::vector<std::string> SupportedSinks;
	AsyncToSync::SynchronousSession SyncSess;
	AsyncMsgs::AsynchronousSession ASyncSess;
	WS::WSConnection *Connection;
	std::string Host;
	std::string HostOS;
	time_t TimeoutSecs;
	int NumAttempts;
	Coyote::UnitType UType;
	
	const std::unordered_map<std::string, msgpack::object> PerformSyncedCommand(const std::string &CommandName, msgpack::zone &TempZone, Coyote::StatusCode *StatusOut = nullptr, const msgpack::object *Values = nullptr);
	Coyote::StatusCode CreatePreset_Multi(const Coyote::Preset &Ref, const std::string &Cmd);
	
	static bool OnMessageReady(WS::WSConnection *Conn, WSMessage *Msg);
	static bool CheckWSInit(void);
	
	inline bool SupportsSink(const std::string &SinkName) const
	{
		for (const std::string &Sink : this->SupportedSinks)
		{
			if (Sink == SinkName) return true;
		}
		
		return false;
	}
	
	inline bool ConfigConnection(bool *SeriousError = nullptr)
	{
		this->CheckWSInit();
		
		WS::WSCore *Core =  WS::WSCore::GetInstance();
		
		if (this->Connection)
		{
			Core->ForgetConnection(this->Connection);
		}
		
		this->SyncSess.DestroyAllTickets();
		
		this->Connection = Core->NewConnection(this->Host, this);

		if (!this->Connection) return false;
		
		msgpack::zone TempZone;
		Coyote::StatusCode S = Coyote::COYOTE_STATUS_INVALID;
		
		//Get unit type, that helps determine what commands we actually want to send the server.
		std::unordered_map<std::string, msgpack::object> Msg { this->PerformSyncedCommand("GetUnitType", TempZone, &S) };
	
		if (S != Coyote::COYOTE_STATUS_OK) return false;
		
		std::unordered_map<std::string, msgpack::object> Data;
		Msg.at("Data").convert(Data);
		
		this->UType = static_cast<Coyote::UnitType>(Data.at("UnitType").as<int>());
		
		//Get host OS
		Msg = this->PerformSyncedCommand("GetHostOS", TempZone, &S);
		
		if (S != Coyote::COYOTE_STATUS_OK) return false;
		
		Data.clear();
		Msg.at("Data").convert(Data);
		
		assert(Data.count("HostOS"));
		
		this->HostOS = Data.at("HostOS").as<std::string>();
		
		//Get supported sinks
		Msg = this->PerformSyncedCommand("GetSupportedSinks", TempZone, &S);
		
		if (S != Coyote::COYOTE_STATUS_OK) return false;
		
		Data.clear();
		Msg.at("Data").convert(Data);
		
		this->SupportedSinks.clear();
		
		Data.at("SupportedSinks").convert(this->SupportedSinks);
		
		static const char *const SubCommands[] = { "SubscribeTC", "SubscribeAssets", "SubscribePresetStates", "SubscribePresets", "!SubscribeHWState", "SubscribePlaybackEvents", nullptr };
		
		for (const char *const *Cmd = SubCommands; *Cmd; ++Cmd)
		{
			const char *Text = *Cmd;
			
			if (*Text == '!')
			{
				if (!this->SupportsSink("kona")) continue;
				++Text;
			}
			
			S = Coyote::COYOTE_STATUS_INVALID;

			this->PerformSyncedCommand(Text, TempZone, &S);
			
			if (S != Coyote::COYOTE_STATUS_OK)
			{
				std::cerr << "libcoyote: Connection registration command \"" << Text << "\" for host " << this->Host << " has failed." << std::endl;
				
				Core->ForgetConnection(this->Connection);
				this->Connection = nullptr;
				
				this->SyncSess.DestroyAllTickets();
				
				if (SeriousError) *SeriousError = true;
				
				return false;
			}
		}
		

		return true;
	}
	
	inline InternalSession(const std::string &Host = "", const int NumAttempts = -1)
		: Connection(),
		Host(Host),
		TimeoutSecs(Coyote::Session::DefaultCommandTimeoutSecs), //10 second default operation timeout
		NumAttempts(NumAttempts),
		UType()
	{
		for (int TryCount = 0; this->NumAttempts == -1 || TryCount < this->NumAttempts; ++TryCount)
		{
			std::cout << "libcoyote: Attempting to connect new session, attempt " << TryCount + 1 << " of " << (this->NumAttempts == -1 ? "infinite" : std::to_string(this->NumAttempts)) << std::endl;
			
			bool SeriousError{};
			
			if (this->ConfigConnection(&SeriousError)) break;
			
			if (SeriousError)
			{
				std::cerr << "libcoyote: Connection to host " << this->Host << " encountered a serious error that prevents another automatic reconnection attempt." << std::endl;
				break;
			}
		}
	}
	inline ~InternalSession(void)
	{
		WS::WSCore::GetInstance()->ForgetConnection(this->Connection);
	}
};



bool InternalSession::CheckWSInit(void)
{
	static std::atomic_bool Entered;
	
	if (!Entered.exchange(true))
	{
		WS::WSCore::Fireup(&InternalSession::OnMessageReady);
		return false;
	}
	
	return true;
}

bool InternalSession::OnMessageReady(WS::WSConnection *Conn, WSMessage *Msg)
{
	InternalSession *Sess = static_cast<InternalSession*>(Conn->UserData);
	
	
	std::unordered_map<std::string, msgpack::object> Values;
	msgpack::zone TempZone;
	
	try
	{
		LDEBUG_MSG("Decoding WSMessage of size " << Msg->GetBodySize());

		Values = MsgpackProc::InitIncomingMsg(Msg->GetBody(), Msg->GetBodySize(), TempZone);
		
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


const std::unordered_map<std::string, msgpack::object> InternalSession::PerformSyncedCommand(const std::string &CommandName, msgpack::zone &TempZone, Coyote::StatusCode *StatusOut, const msgpack::object *Values)
{
	msgpack::sbuffer Buffer;
	msgpack::packer<msgpack::sbuffer> Pack { Buffer };
	
	//Acquire a new message ID
	const uint64_t MsgID = this->SyncSess.NewMsgID();
	
#ifdef LCVERBOSE
	if (Values)
	{
		LDEBUG_MSG("Outgoing data is " << *Values);
	}
#endif //LCVERBOSE

	//Pack our values into a msgpack buffer
	MsgpackProc::InitOutgoingMsg(Pack, CommandName, MsgID, Values);
	
	if (!this->Connection || this->Connection->HasError())
	{	
		if (StatusOut) *StatusOut = Coyote::COYOTE_STATUS_NETWORKERROR;
		
		return {};
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
	const std::unordered_map<std::string, msgpack::object> Results { MsgpackProc::InitIncomingMsg(ResponsePtr->GetBody(), ResponsePtr->GetBodySize(), TempZone) };
	
	//Get the status code.
	assert(Results.count("StatusInt"));
	
	if (StatusOut) *StatusOut = static_cast<Coyote::StatusCode>(Results.at("StatusInt").as<int>());
	
	return Results;
}

Coyote::Session::Session(const std::string &Host, const int NumAttempts) : Internal(new InternalSession{Host, NumAttempts})
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

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK) };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("Take", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetMaxCPUPercentage(uint8_t &ValueOut)
{
	DEF_SESS;
	
	msgpack::zone TempZone;
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetMaxCPUPercentage", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;

	std::unordered_map<std::string, msgpack::object> DataField;

	Msg.at("Data").convert(DataField);

	ValueOut = (uint8_t)DataField.at("MaxCPUPercentage").as<uint32_t>();

	return Status;
}

Coyote::StatusCode Coyote::Session::SetMaxCPUPercentage(const uint8_t MaxCPUPercentage)
{
	DEF_SESS;

	msgpack::zone TempZone;

	Coyote::StatusCode Status{};

	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(MaxCPUPercentage) };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("SetMaxCPUPercentage", TempZone, &Status, &Pass);

	return Status;
}

Coyote::StatusCode Coyote::Session::SelectNext(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("SelectNext", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::LoadNetSettings(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("LoadNetSettings", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SelectPrev(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("SelectPrev", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::TakeNext(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("TakeNext", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::TakePrev(void)
{
	DEF_SESS;

	msgpack::zone TempZone;
	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("TakePrev", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::Pause(const int32_t PK)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK) };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("Pause", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetPausedState(const int32_t PK, const bool Value)
{
	DEF_SESS;

	msgpack::zone TempZone;
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK) };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand(Value ? "SetPause" : "UnsetPause", TempZone, &Status, &Pass);
	
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

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("End", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeleteAsset(const std::string &FullPath)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "FullPath", msgpack::object{FullPath.c_str(), TempZone } } };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	SESS.PerformSyncedCommand("DeleteAsset", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::InstallAsset(const std::string &FullPath, const std::string &OutputDir)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	std::unordered_map<std::string, msgpack::object> Values { { "FullPath",  msgpack::object{FullPath.c_str(), TempZone } } };
	
	if (!OutputDir.empty())
	{
		Values.emplace("OutputDirectory", msgpack::object{ OutputDir.c_str(), TempZone });
	}
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("InstallAsset", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SubscribeMiniview(const int32_t PK)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	std::unordered_map<std::string, msgpack::object> Values { { "PK", msgpack::object{ PK, TempZone } } };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("SubscribeMiniview", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::UnsubscribeMiniview(const int32_t PK)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	std::unordered_map<std::string, msgpack::object> Values { { "PK", msgpack::object{ PK, TempZone } } };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("UnsubscribeMiniview", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RenameAsset(const std::string &FullPath, const std::string &NewName)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "FullPath", msgpack::object{FullPath.c_str(), TempZone } }, { "NewName", msgpack::object{NewName.c_str(), TempZone} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("RenameAsset", TempZone, &Status, &Pass);
	
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

Coyote::StatusCode InternalSession::CreatePreset_Multi(const Coyote::Preset &Ref, const std::string &Cmd)
{
	Coyote::StatusCode Status{};

	msgpack::zone TempZone;
	
	const msgpack::object &Data = MsgpackProc::PackCoyoteObject(&Ref, TempZone);

	this->PerformSyncedCommand(Cmd, TempZone, &Status, &Data);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RenameCountdown(const int32_t PK, const int32_t Time, const std::string &NewName)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, TempZone} },
		{ "TimeMS", msgpack::object{Time, TempZone} },
		{ "NewName", msgpack::object{NewName.c_str(), TempZone} }
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("RenameCountdown", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RenameGoto(const int32_t PK, const int32_t Time, const std::string &NewName)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, TempZone} },
		{ "TimeMS", msgpack::object{Time, TempZone} },
		{ "NewName", msgpack::object{NewName.c_str(), TempZone} }
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("RenameGoto", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeleteCountdown(const int32_t PK, const int32_t Time)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, TempZone} },
		{ "TimeMS", msgpack::object{Time, TempZone} },
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("DeleteCountdown", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeleteGoto(const int32_t PK, const int32_t Time)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, TempZone} },
		{ "TimeMS", msgpack::object{Time, TempZone} },
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("DeleteGoto", TempZone, &Status, &Pass);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::CreateCountdown(const int32_t PK, const int32_t Time, const std::string &Name)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, TempZone} },
		{ "TimeMS", msgpack::object{Time, TempZone} },
		{ "Name", msgpack::object{Name.c_str(), TempZone} }
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("CreateCountdown", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::CreateGoto(const int32_t PK, const int32_t Time, const std::string &Name)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "PK", msgpack::object{PK, TempZone} },
		{ "TimeMS", msgpack::object{Time, TempZone} },
		{ "Name", msgpack::object{Name.c_str(), TempZone} }
	};
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("CreateGoto", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::BeginUpdate(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("BeginUpdate", TempZone, &Status);
	
	return Status;
}


Coyote::StatusCode Coyote::Session::RebootCoyote(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("RebootCoyote", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SoftRebootCoyote(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("SoftRebootCoyote", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ShutdownCoyote(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("ShutdownCoyote", TempZone, &Status);
	
	return Status;
}

	
Coyote::StatusCode Coyote::Session::IsMirror(bool &ValueOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	const char *CmdName = "IsMirror";
	
	Coyote::StatusCode Status{};

	const std::unordered_map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand(CmdName, TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> DataField;
	
	Response.at("Data").convert(DataField);
		
	ValueOut = DataField[CmdName].as<bool>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetIsServerUnit(bool &ValueOut)
{
	DEF_SESS;

	ValueOut = SESS.UType != COYOTE_UTYPE_FLEX;
	
	return COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetSupportsS12G(bool &ValueOut)
{
	DEF_SESS;

	if (!SESS.SupportsSink("kona"))
	{
		return COYOTE_STATUS_UNSUPPORTED;
	}
	
	msgpack::zone TempZone;	
	const char *CmdName = "GetSupportsS12G";
	
	Coyote::StatusCode Status{};

	const std::unordered_map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand(CmdName, TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;	
	
	std::unordered_map<std::string, msgpack::object> DataField;
	
	Response.at("Data").convert(DataField);
		
	ValueOut = DataField["SupportsS12G"].as<bool>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SynchronizerBusy(bool &ValueOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	const char *CmdName = "SynchronizerBusy";
	
	Coyote::StatusCode Status{};

	const std::unordered_map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand(CmdName, TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;	
	
	std::unordered_map<std::string, msgpack::object> DataField;
	
	Response.at("Data").convert(DataField);
		
	ValueOut = DataField["Busy"].as<bool>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeconfigureSync(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	SESS.PerformSyncedCommand("DeconfigureSync", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::AddMirror(const std::string &MirrorIP)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "IP",  msgpack::object{MirrorIP.c_str(), TempZone } } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("AddMirror", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::StartSpoke(const std::string &SpokeName)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "SpokeName",  msgpack::object{SpokeName.c_str(), TempZone } } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("StartSpoke", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::KillSpoke(const std::string &SpokeName)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "SpokeName",  msgpack::object{SpokeName.c_str(), TempZone } } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("KillSpoke", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::RestartSpoke(const std::string &SpokeName)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "SpokeName",  msgpack::object{SpokeName.c_str(), TempZone } } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("RestartSpoke", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetSupportedSinks(std::vector<std::string> &Out)
{
	DEF_SESS;
	
	Out = SESS.SupportedSinks;

	return COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetDisks(std::vector<Coyote::Drive> &Out)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	const char *CmdName = "GetDisks";
	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand(CmdName, TempZone, &Status) };
	
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
		std::unique_ptr<Coyote::Drive> Obj { static_cast<Coyote::Drive*>(MsgpackProc::UnpackCoyoteObject(Item, typeid(Coyote::Drive))) };
	
		assert(Obj.get() != nullptr);
		
		Out.push_back(std::move(*Obj));
	}
	
	return Status;
}

Coyote::StatusCode Coyote::Session::EjectDisk(const std::string &Mountpoint)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "Mountpoint", msgpack::object{Mountpoint.c_str()} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	const std::unordered_map<std::string, msgpack::object> &Response { SESS.PerformSyncedCommand("EjectDisk", TempZone, &Status, &Pass) };
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ReorderPresets(const int32_t PK1, const int32_t PK2)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK1), MAPARG(PK2) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("ReorderPresets", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::MovePreset(const int32_t PK, const std::string TabID, const uint32_t NewIndex)
{

	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK), MAPARG(TabID), MAPARG(NewIndex) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("MovePreset", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeletePreset(const int32_t PK)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	Coyote::StatusCode Status{};
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("DeletePreset", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SeekTo(const int32_t PK, const uint32_t TimeIndex)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK), MAPARG(TimeIndex) };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	Coyote::StatusCode Status{};

	SESS.PerformSyncedCommand("SeekTo", TempZone, &Status, &Pass);
	
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
	
Coyote::StatusCode Coyote::Session::GetPresetStates(std::vector<Coyote::PresetState> &Out)
{
	DEF_SESS;

	std::unique_ptr<std::unordered_map<int32_t, Coyote::PresetState> > Ptr { SESS.ASyncSess.SubSession.GetPresetStates() };
	
	if (!Ptr) return Coyote::COYOTE_STATUS_FAILED;
	
	Out.clear();
	Out.reserve(Ptr->size());
	
	for (auto Pair : *Ptr)
	{
		Out.push_back(std::move(Pair.second));
	}

	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetPresetStatesMap(std::unordered_map<int32_t, Coyote::PresetState> &Out)
{
	DEF_SESS;

	std::unique_ptr<std::unordered_map<int32_t, Coyote::PresetState> > Ptr { SESS.ASyncSess.SubSession.GetPresetStates() };
	
	if (!Ptr) return Coyote::COYOTE_STATUS_FAILED;

	Out = std::move(*Ptr);

	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetPresetsMap(std::unordered_map<int32_t, Coyote::Preset> &Out)
{
	DEF_SESS;

	std::unique_ptr<std::unordered_map<int32_t, Coyote::Preset> > Ptr { SESS.ASyncSess.SubSession.GetPresets() };
	
	if (!Ptr) return Coyote::COYOTE_STATUS_FAILED;
	
	Out = std::move(*Ptr);
	
	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetTimeCodesMap(std::unordered_map<int32_t, Coyote::TimeCode> &Out)
{
	DEF_SESS;

	std::unique_ptr<std::unordered_map<int32_t, Coyote::TimeCode> > Ptr { SESS.ASyncSess.SubSession.GetTimeCodesMap() };
	
	if (!Ptr) return Coyote::COYOTE_STATUS_FAILED;
	
	Out = std::move(*Ptr);
	
	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetPresets(std::vector<Coyote::Preset> &Out)
{
	DEF_SESS;

	std::unique_ptr<std::unordered_map<int32_t, Coyote::Preset> > Ptr { SESS.ASyncSess.SubSession.GetPresets() };
	
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

	std::unique_ptr<std::unordered_map<std::string, Coyote::Asset> > Ptr { SESS.ASyncSess.SubSession.GetAssets() };
	
	if (!Ptr) return Coyote::COYOTE_STATUS_FAILED;
	
	Out.clear();
	Out.reserve(Ptr->size());
	
	for (auto Pair : *Ptr)
	{
		Out.push_back(std::move(Pair.second));
	}

	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetKonaHardwareState(Coyote::KonaHardwareState &Out)
{
	DEF_SESS;

	if (!SESS.SupportsSink("kona"))
	{
		return COYOTE_STATUS_UNSUPPORTED;
	}
	
	std::unique_ptr<Coyote::KonaHardwareState> Ptr;
	try
	{
		Ptr.reset(SESS.ASyncSess.SubSession.GetKonaHardwareState());
	}
	catch (...)
	{
		return Coyote::COYOTE_STATUS_FAILED;
	}
	
	if (!Ptr) return Coyote::COYOTE_STATUS_FAILED;
	
	Out = std::move(*Ptr);
	
	return Coyote::COYOTE_STATUS_OK;
}


bool Coyote::Session::Connected(void) const
{
	DEF_CONST_SESS;
	
	return SESS.Connection && !this->HasConnectionError();
}

bool Coyote::Session::Reconnect(const std::string &Host)
{
	DEF_SESS;

	if (!Host.empty())
	{
		SESS.Host = Host;
	}
	
	for (int TryCount = 0; (SESS.NumAttempts == -1 || TryCount < SESS.NumAttempts); ++TryCount)
	{
		std::cout << "libcoyote: Manually attempting to reconnect, attempt " << TryCount + 1 << " of " << (SESS.NumAttempts == -1 ? "infinite" : std::to_string(SESS.NumAttempts)) << std::endl;
		
		bool SeriousError{};
		
		if (SESS.ConfigConnection(&SeriousError)) return true;
		
		if (SeriousError)
		{
			std::cerr << "libcoyote: Manual reconnection attempt failed due to a serious error during negotiation." << std::endl;
			return false;
		}
	}
	
	return false;
}

std::vector<Coyote::LANCoyote> Coyote::GetLANCoyotes(void)
{
	InternalSession::CheckWSInit();

	return Discovery::DiscoverySession::CheckInit()->GetLANCoyotes();
}

std::string Coyote::Session::GetHost(void) const
{
	DEF_CONST_SESS;
	
	return SESS.Host;
}

std::string Coyote::Session::GetHostOS(void) const
{
	DEF_CONST_SESS;
	
	return SESS.HostOS;
}

Coyote::StatusCode Coyote::Session::GetIP(const int32_t AdapterID, Coyote::NetworkInfo &Out)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(AdapterID) };
	
	Coyote::StatusCode Status{};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetIP", TempZone, &Status, &Pass) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unique_ptr<Coyote::NetworkInfo> Ptr { static_cast<Coyote::NetworkInfo*>(MsgpackProc::UnpackCoyoteObject(Msg.at("Data"), typeid(Coyote::NetworkInfo))) };
	
	Out = std::move(*Ptr);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::ReadAssetMetadata(const std::string &FullPath, Coyote::AssetMetadata &Out)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(FullPath) };
	
	Coyote::StatusCode Status{};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("ReadAssetMetadata", TempZone, &Status, &Pass) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unique_ptr<Coyote::AssetMetadata> Ptr { static_cast<Coyote::AssetMetadata*>(MsgpackProc::UnpackCoyoteObject(Msg.at("Data"), typeid(Coyote::AssetMetadata))) };
	
	Out = std::move(*Ptr);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetIP(const Coyote::NetworkInfo &Input)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	const std::unordered_map<std::string, msgpack::object> Values { { "AdapterID", msgpack::object{Input.AdapterID} }, { "Subnet", msgpack::object{Input.Subnet.c_str()} }, { "IP", msgpack::object{Input.IP.c_str()} } };
	
	Coyote::StatusCode Status{};

	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	SESS.PerformSyncedCommand("SetIP", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SelectPreset(const int32_t PK)
{
	DEF_SESS;

	msgpack::zone TempZone;
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(PK) };

	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	SESS.PerformSyncedCommand("SelectPreset", TempZone, &Status, &Pass);
	
	return Status;
}


Coyote::StatusCode Coyote::Session::GetGenlockSettings(Coyote::GenlockSettings &Out)
{
	DEF_SESS;

	if (!SESS.SupportsSink("kona"))
	{
		return COYOTE_STATUS_UNSUPPORTED;
	}
	
	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetGenlockSettings", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	
	std::unique_ptr<Coyote::GenlockSettings> Ptr { static_cast<Coyote::GenlockSettings*>(MsgpackProc::UnpackCoyoteObject(Msg.at("Data"), typeid(Coyote::GenlockSettings))) };
	
	Out = *Ptr;
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetVertGenlock(int32_t VertValue)
{
	DEF_SESS;

	if (!SESS.SupportsSink("kona"))
	{
		return COYOTE_STATUS_UNSUPPORTED;
	}
	
	msgpack::zone TempZone;
	
	StatusCode Status = COYOTE_STATUS_INVALID;
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(VertValue) };

	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("SetVertGenlock", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetHorzGenlock(int32_t HorzValue)
{
	DEF_SESS;

	if (!SESS.SupportsSink("kona"))
	{
		return COYOTE_STATUS_UNSUPPORTED;
	}
		
	msgpack::zone TempZone;
	
	StatusCode Status = COYOTE_STATUS_INVALID;
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(HorzValue) };

	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("SetHorzGenlock", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetKonaHardwareMode(const std::array<ResolutionMode, NUM_KONA_OUTS> &Resolutions,
														const RefreshMode RefreshRate,
														const HDRMode HDRMode,
														const EOTFMode EOTFSetting,
														const bool ConstLumin,
														const KonaAudioConfig AudioConfig)
{
	DEF_SESS;

	if (!SESS.SupportsSink("kona"))
	{
		return COYOTE_STATUS_UNSUPPORTED;
	}
	
	msgpack::zone TempZone;	
	assert(RefreshMap.count(RefreshRate));
	
	StatusCode Status{};
	
	std::vector<msgpack::object> ResolutionStrings;
	
	ResolutionStrings.reserve(NUM_KONA_OUTS);
	
	for (size_t Inc = 0u; Inc < NUM_KONA_OUTS; ++Inc)
	{
		ResolutionStrings.emplace_back(ResolutionMap.at(Resolutions[Inc]).c_str());
	}
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "Resolutions", msgpack::object{MsgpackProc::STLArrayToMsgpackArray(ResolutionStrings, TempZone)} },
		{ "RefreshRate", msgpack::object{RefreshMap.at(RefreshRate).c_str()} },
		{ "HDRMode", msgpack::object{ (int)HDRMode } },
		{ "EOTFSetting", msgpack::object{ (int)EOTFSetting } },
		{ "ConstLumin", msgpack::object{ ConstLumin } },
		{ "AudioConfig", msgpack::object{ (int)AudioConfig } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("SetKonaHardwareMode", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::UploadState(const std::string &PresetsJson, const std::string &SettingsJson)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "PresetsJson", msgpack::object{ PresetsJson.c_str() } },
		{ "SettingsJson", msgpack::object{ SettingsJson.c_str() } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("UploadState", TempZone, &Status, &Pass);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::DeleteWatchPath(const std::string &Path)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "Path", msgpack::object{ Path.c_str() } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("DeleteWatchPath", TempZone, &Status, &Pass);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::ManualForgetAsset(const std::string &Path)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "FullPath", msgpack::object{ Path.c_str() } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("ManualForgetAsset", TempZone, &Status, &Pass);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::ManualAddAsset(const std::string &Path)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "FullPath", msgpack::object{ Path.c_str() } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("ManualAddAsset", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ExitSupervisor(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	SESS.PerformSyncedCommand("ExitSupervisor", TempZone, &Status);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::AddWatchPath(const std::string &Path)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "Path", msgpack::object{ Path.c_str() } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("AddWatchPath", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DownloadState(std::string &PresetsJsonOut, std::string &SettingsJsonOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("DownloadState", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("PresetsJson") && Data.count("SettingsJson"));
	
	PresetsJsonOut = Data.at("PresetsJson").as<std::string>();
	SettingsJsonOut = Data.at("SettingsJson").as<std::string>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetCurrentRole(Coyote::UnitRole &CurrentRoleOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetCurrentRole", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("CurrentRoleInt") && Data.count("CurrentRoleText"));
	
	CurrentRoleOut = static_cast<UnitRole>(Data.at("CurrentRoleInt").as<int>());
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ActivateMachine(const std::string &LicenseKey)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};

	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "LicenseKey", msgpack::object{ LicenseKey.c_str() } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("ActivateMachine", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::DeactivateMachine(const std::string &LicenseKey, const std::string &LicenseMachineUUID)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};

	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "LicenseKey", msgpack::object{ LicenseKey.c_str() } },
		{ "LicenseMachineUUID", msgpack::object{ LicenseMachineUUID.c_str() } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("DeactivateMachine", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetLicensingStatus(LicensingStatus &LicStats)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};

	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetLicensingStatus", TempZone, &Status) };

	if (Status != Coyote::COYOTE_STATUS_OK) return Status;

	std::unordered_map<std::string, msgpack::object> Data;

	Msg.at("Data").convert(LicStats);

	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::HostSinkEarlyFireup(void)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};

	SESS.PerformSyncedCommand("HostSinkEarlyFireup", TempZone, &Status);

	return Status;
}

Coyote::StatusCode Coyote::Session::GetLicenseType(const std::string &LicenseKey, std::string &LicenseTypeOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};

	const std::unordered_map<std::string, msgpack::object> Values
	{
		{ "LicenseKey", msgpack::object{ LicenseKey.c_str() } },
	};
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetLicenseType", TempZone, &Status, &Pass) };

	if (Status != Coyote::COYOTE_STATUS_OK) return Status;

	std::unordered_map<std::string, msgpack::object> Data;

	Msg.at("Data").convert(Data);

	LicenseTypeOut = Data["LicenseType"].as<std::string>();
	
	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetUnitID(std::string &UnitIDOut, std::string &NicknameOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetUnitID", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("UnitID") && Data.count("Nickname"));
	
	UnitIDOut = Data.at("UnitID").as<std::string>();
	NicknameOut = Data.at("Nickname").as<std::string>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetServerVersion(std::string &Out)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetServerVersion", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("Version"));
	
	Out = Data.at("Version").as<std::string>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetHostSinkResolution(const uint32_t HDMINum, Coyote::ResolutionMode &ResOut, Coyote::RefreshMode &FPSOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(HDMINum) };
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetHostSinkResolution", TempZone, &Status, &Pass) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("Res") && Data.count("FPS"));
	
	ResOut = ReverseResolutionMap(Data.at("Res").as<std::string>());
	FPSOut = ReverseRefreshMap(Data.at("FPS").as<std::string>());
	
	return Status;
}

Coyote::StatusCode Coyote::Session::SetHostSinkResolution(const uint32_t HDMINum, const Coyote::ResolutionMode Res, const Coyote::RefreshMode FPS)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(HDMINum), { "Res", msgpack::object{ ResolutionMap.at(Res), TempZone } }, { "FPS", msgpack::object { RefreshMap.at(FPS), TempZone } } };
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("SetHostSinkResolution", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetBMDResolution(const uint32_t SDIIndex, Coyote::ResolutionMode &ResOut, Coyote::RefreshMode &FPSOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(SDIIndex) };
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetBMDResolution", TempZone, &Status, &Pass) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	assert(Data.count("Res") && Data.count("FPS"));
	
	ResOut = ReverseResolutionMap(Data.at("Res").as<std::string>());
	FPSOut = ReverseRefreshMap(Data.at("FPS").as<std::string>());
	
	return Status;
}
Coyote::StatusCode Coyote::Session::SetBMDResolution(const uint32_t SDIIndex, const Coyote::ResolutionMode Res, const Coyote::RefreshMode FPS)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(SDIIndex), { "Res", msgpack::object{ ResolutionMap.at(Res), TempZone } }, { "FPS", msgpack::object { RefreshMap.at(FPS), TempZone } } };
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	SESS.PerformSyncedCommand("SetBMDResolution", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetDiskAssets(std::vector<ExternalAsset> &Out, const std::string &DriveName, const std::string &Subpath)
{
	DEF_SESS;

	msgpack::zone TempZone;
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { MAPARG(DriveName), MAPARG(Subpath) };
	
	const msgpack::object &Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetDiskAssets", TempZone, &Status, &Pass) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::vector<msgpack::object> Data;

	Msg.at("Data").convert(Data);
	
	for (msgpack::object &Obj : Data)
	{
		std::unique_ptr<Coyote::ExternalAsset> DAStruct { static_cast<Coyote::ExternalAsset*>(MsgpackProc::UnpackCoyoteObject(Obj, typeid(Coyote::ExternalAsset))) };
		
		Out.emplace_back(std::move(*DAStruct));
	}

	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetWatchPaths(std::vector<std::string> &Out)
{
	DEF_SESS;

	msgpack::zone TempZone;
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetWatchPaths", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	Msg.at("Data").convert(Out);
	
	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetMirrors(std::vector<Coyote::Mirror> &Out)
{
	DEF_SESS;

	msgpack::zone TempZone;
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetMirrors", TempZone, &Status) };
	
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

Coyote::StatusCode Coyote::Session::GetDesignatedPrimary(Coyote::Mirror &Out)
{
	DEF_SESS;

	msgpack::zone TempZone;
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetDesignatedPrimary", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unique_ptr<Coyote::Mirror> MirrorStruct { static_cast<Coyote::Mirror*>(MsgpackProc::UnpackCoyoteObject(Msg.at("Data"), typeid(Coyote::Mirror))) };

	Out = *MirrorStruct;
	
	return Coyote::COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::GetEffectivePrimary(Coyote::Mirror &Out)
{
	DEF_SESS;

	msgpack::zone TempZone;
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetEffectivePrimary", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unique_ptr<Coyote::Mirror> MirrorStruct { static_cast<Coyote::Mirror*>(MsgpackProc::UnpackCoyoteObject(Msg.at("Data"), typeid(Coyote::Mirror))) };

	Out = *MirrorStruct;
	
	return Coyote::COYOTE_STATUS_OK;
}


Coyote::StatusCode Coyote::Session::GetUnitType(UnitType &Out)
{
	DEF_SESS;

	Out = SESS.UType;
	
	return COYOTE_STATUS_OK;
}

Coyote::StatusCode Coyote::Session::DetectUpdate(bool &DetectedOut, std::string *Out)
{
	DEF_SESS;

	msgpack::zone TempZone;
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("DetectUpdate", TempZone, &Status) };

	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;
	Msg.at("Data").convert(Data);
	
	DetectedOut = Data.at("IsUpdateDetected").as<bool>();
	
	if (Out) *Out = Data.at("Version").as<std::string>();
	
	return Status;
}

Coyote::StatusCode Coyote::Session::GetLogsZip(std::vector<uint8_t> &OutBuffer)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetLogsZip", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;

	Msg.at("Data").convert(Data);
	
	Data.at("ZipBytes").convert(OutBuffer);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::GetKonaAVBufferLevels(std::vector<std::tuple<uint32_t, uint32_t, int64_t>> &OutLevels)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("GetKonaAVBufferLevels", TempZone, &Status) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;

	Msg.at("Data").convert(OutLevels);
	
	return Status;
}

bool Coyote::Session::HasConnectionError(void) const
{
	DEF_SESS;
	
	return SESS.Connection->HasError();
}

void Coyote::Session::SetMiniviewCallback(const MiniviewCallback CB, void *const UserData)
{
	DEF_SESS;
	
	SESS.ASyncSess.SetMiniviewCallback(CB, UserData);
}

void Coyote::Session::SetPlaybackEventCallback(const PBEventCallback CB, void *const UserData)
{
	DEF_SESS;
	
	SESS.ASyncSess.SetPlaybackEventCallback(CB, UserData);
}

void Coyote::Session::SetStateEventCallback(const StateEventType EType, const StateEventCallback CB, void *const UserData)
{
	DEF_SESS;
	
	SESS.ASyncSess.SetStateEventCallback(EType, CB, UserData);
}
	
Coyote::StatusCode Coyote::Session::ReadLog(const std::string &SpokeName, const int Year, const int Month, const int Day, std::string &LogOut)
{
	DEF_SESS;

	msgpack::zone TempZone;	
	StatusCode Status{};
	

	LDEBUG_MSG("Building values");
	const std::unordered_map<std::string, msgpack::object> Values { {"SpokeName", msgpack::object{SpokeName.c_str()} }, { "Year", msgpack::object{Year} }, { "Month", msgpack::object{Month} }, { "Day", msgpack::object{Day} } };
	
	LDEBUG_MSG("Performing conversions");
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };
	
	LDEBUG_MSG("Executing method");
	const std::unordered_map<std::string, msgpack::object> &Msg { SESS.PerformSyncedCommand("ReadLog", TempZone, &Status, &Pass) };
	
	if (Status != Coyote::COYOTE_STATUS_OK) return Status;
	
	std::unordered_map<std::string, msgpack::object> Data;

	LDEBUG_MSG("Converting Data");
	Msg.at("Data").convert(Data);
	
	LDEBUG_MSG("Converting LogText");
	Data.at("LogText").convert(LogOut);

	LDEBUG_MSG("Success");
	return Status;
}

Coyote::StatusCode Coyote::Session::SetUnitNickname(const std::string &Nickname)
{
	DEF_SESS;

	msgpack::zone TempZone;

	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "Nickname", msgpack::object{Nickname.c_str()} } };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("SetUnitNickname", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::ExportLogsZip(const std::string &Mountpoint)
{
	DEF_SESS;

	msgpack::zone TempZone;

	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "Mountpoint", msgpack::object{Mountpoint.c_str()} } };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("ExportLogsZip", TempZone, &Status, &Pass);
	
	return Status;
}
	
Coyote::StatusCode Coyote::Session::_SVS_WriteCytLog_(const std::string &Param1, const std::string &Param2)
{ //This function is intended for Sonoran Video Systems use only. Use by users will either not work or just confuse unit logs.
	DEF_SESS;

	msgpack::zone TempZone;	

	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "SpokeName", msgpack::object{Param1.c_str()} }, { "LogText", msgpack::object{Param2.c_str()} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("__LOGWRITE__", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::_SVS_RegisterPing_(const std::string &Param1)
{ //This function is intended for Sonoran Video Systems use only. Use by users will either not work or just confuse unit logs.
	DEF_SESS;

	msgpack::zone TempZone;	

	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "SpokeName", msgpack::object{Param1.c_str()} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("__REGISTER_PING__", TempZone, &Status, &Pass);
	
	return Status;
}

Coyote::StatusCode Coyote::Session::_SVS_RegisterReady_(const std::string &Param1)
{ //This function is intended for Sonoran Video Systems use only. Use by users will either not work or just confuse unit logs.
	DEF_SESS;

	msgpack::zone TempZone;	

	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "SpokeName", msgpack::object{Param1.c_str()} } };
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("__REGISTER_READY__", TempZone, &Status, &Pass);
	
	return Status;
}
Coyote::StatusCode Coyote::Session::_SVS_APID_(const std::string &Param1, const int64_t Param2)
{ //This function is intended for Sonoran Video Systems use only. Use by users will either not work or just confuse unit logs.
	DEF_SESS;

	msgpack::zone TempZone;	

	StatusCode Status{};
	
	const std::unordered_map<std::string, msgpack::object> Values { { "SpokeName", msgpack::object{Param1.c_str()} }, { "AssocPID", msgpack::object{Param2} } };
	
	const msgpack::object Pass { MsgpackProc::STLMapToMsgpackMap(Values, TempZone) };

	SESS.PerformSyncedCommand("__SVS_APID__", TempZone, &Status, &Pass);
	
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
