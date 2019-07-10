#include <libwebsockets.h>
#include <arpa/inet.h>
#include "include/internal/native_ws.h"
#include "include/internal/common.h"


/**
 * By the time you are done with this code, you will have developed a deep, seething hatred for WebSockets, libwebsockets, and forced asynchronous network programming.
 **/
 
#define PRE_WS_START LWS_SEND_BUFFER_PRE_PADDING
#define PRE_WS_END LWS_SEND_BUFFER_POST_PADDING
#define PRE_WS_SIZE (PRE_WS_START + PRE_WS_END)

//Prototypes

//Globals
const struct lws_protocols WS::WSConnection::WSProtocols[] = { { "", WS::WSConnection::WSCallback, 0, 16384}, { } };

//Implementations


bool WS::WSConnection::InitThreadClient(void)
{
	this->ClearError();

	//Bring up the socket.
	for (uint8_t Attempts = 3; Attempts && !this->Socket; --Attempts)
	{
		struct lws_client_connect_info ConnectInfo{};
		
		ConnectInfo.context = this->Ctx;
		ConnectInfo.address = this->Host.c_str();
		ConnectInfo.port = WS::PortNum;
		ConnectInfo.path = "/";
		ConnectInfo.host = lws_canonical_hostname(this->Ctx);
		ConnectInfo.origin = this->Host.c_str();
		ConnectInfo.protocol = this->WSProtocols->name;
		ConnectInfo.userdata = this;
		
		this->Socket = lws_client_connect_via_info(&ConnectInfo);
	}

	if (!this->Socket) return false;
	
	this->RegisterActivity(); //Do it once so we know we're reconnected

	return true;
}
	
void WS::WSConnection::ThreadFunc(void)
{
	this->InitWebSockets();
	
	this->InitThreadClient();
	
	while (!this->ShouldDie)
	{
		lws_service(this->Ctx, 1);
		
		if (this->HasError() || this->CheckPingout())
		{
			std::cerr << "LibCoyote: THREAD DYING for connection " << (void*)this << ", error detected.\n";
			this->ErrorDetected = true;
			return;
		}
	
		if (this->NeedsPing()) this->OnNeedPing(this);
		
		std::unique_lock<std::mutex> G { this->OMutex };
		
		if (!this->Outgoing.empty())
		{
			G.unlock();
			lws_callback_on_writable_all_protocol(this->Ctx, WSProtocols);
		}

	}	
}

bool WS::WSConnection::CheckPingout(void) const
{ //True if we're dead
	return EYEBLEED_NOW_MS() - this->LastPingMS > PingInterval + PingoutMS;
}

bool WS::WSConnection::NeedsPing(void) const
{
	return EYEBLEED_NOW_MS() - this->LastPingMS > PingInterval;
}

bool WS::WSConnection::InitWebSockets(void)
{
	static bool LogConfigured;
	
	if (!LogConfigured)
	{
		lws_set_log_level(0, nullptr);
		LogConfigured = true;
	}
	
	if (this->Ctx) lws_context_destroy(this->Ctx);

	struct lws_context_creation_info CtxInfo{};

	CtxInfo.port = CONTEXT_PORT_NO_LISTEN;
	CtxInfo.protocols = WSProtocols;
	CtxInfo.gid = -1;
	CtxInfo.uid = -1;
	CtxInfo.ka_time = 10;
	CtxInfo.ka_interval = 2;
	CtxInfo.ka_probes = 2;
	CtxInfo.timeout_secs = 2;
	
	this->Ctx = lws_create_context(&CtxInfo);
	
	return (bool)this->Ctx;
}

bool WS::WSConnection::Connect(const std::string &Host)
{
	if (this->Thread)
	{
		this->ShouldDie = true;
		this->Thread->join();
		this->ShouldDie = false;
		delete this->Thread;
	}
	
	this->Host = Host;
		
	this->Thread = new std::thread(&WS::WSConnection::ThreadFunc, this);
	
	return true;
}
	
int WS::WSConnection::WSCallback(struct lws *WSDesc, const enum lws_callback_reasons Reason, void *UserData, void *Data, const size_t DataLength)
{
	WSConnection *ThisPtr = static_cast<WSConnection*>(UserData);
	
	switch (Reason)
	{
		case LWS_CALLBACK_CLIENT_ESTABLISHED:
			ThisPtr->Socket = WSDesc;
			lws_callback_on_writable(WSDesc);
			break;
		case LWS_CALLBACK_RECEIVE:
		case LWS_CALLBACK_CLIENT_RECEIVE:
		{
			ThisPtr->RegisterActivity();
			ThisPtr->AddFragment(Data, DataLength);
			
			lws_callback_on_writable(WSDesc);

			break;
		}
		case LWS_CALLBACK_CLIENT_WRITEABLE:
		{
			std::unique_lock<std::mutex> Guard { ThisPtr->OMutex };
			
			if (ThisPtr->Outgoing.empty())
			{
				break; //Nothing to do.
			}
			
			WSMessage *Msg = ThisPtr->Outgoing.front();
			
			Guard.unlock();
			
			const int MsgSize = Msg->GetRemainingSize() + sizeof(uint32_t);
			int Written = 0, TotalWritten = 0;
			
			uint8_t *DataHead = Msg->GetSeekedBody() - sizeof(uint32_t);
			
			do
			{
				Written = lws_write(WSDesc, DataHead, MsgSize - TotalWritten, LWS_WRITE_BINARY);
				
				if (Written <= 0) continue;
				
				ThisPtr->RegisterActivity(); //Write call didn't fail, so we can count this as a win.
				
				TotalWritten += Written;
				DataHead += Written;
			} while (MsgSize > TotalWritten && Written != -1);
			
			
			if (Written != -1 && TotalWritten < MsgSize)
			{ //Only partially transmitted
				Msg->RawSeekForward(TotalWritten);
				break;
			}
			
			//Problems.
			if (Written == -1)
			{
				ThisPtr->ErrorDetected = true;
				break;
			}
			
			Guard.lock();
			//We made it.
			ThisPtr->Outgoing.pop();
			delete Msg;
			
			if (!ThisPtr->Outgoing.empty() && Written > 0)
			{
				lws_callback_on_writable(WSDesc);
			}
		}
		case LWS_CALLBACK_CLIENT_CLOSED:
		{
			ThisPtr->Socket = nullptr;
			break;
		}
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		{
			ThisPtr->ErrorDetected = true;
			lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
												Data ? (const char*)Data : "No info");
			break;
		}
		default:
			break;
	}
	
	return 0;
}

void WS::WSConnection::AddFragment(const void *Data, const size_t DataSize)
{
	if (!this->RecvFragment)
	{
		this->RecvFragment = new WSMessage::Fragment(Data, DataSize);
	}
	else
	{
		this->RecvFragment->Append(Data, DataSize);
	}
	
	if (this->RecvFragment->IsComplete())
	{
		WSMessage *Msg = this->RecvFragment->Graduate();
		delete this->RecvFragment;
		this->RecvFragment = nullptr;

		this->OnReceiveCallback(this->OnReceiveCallbackUserData, Msg);
		
	}
}

void WS::WSConnection::KillThread(void)
{
	if (!this->Thread) return;
	
	this->ShouldDie = true;
	this->Thread->join();
	this->ShouldDie = false;
	
	delete this->Thread;
	this->Thread = nullptr;
}
	
void WS::WSConnection::Shutdown(void)
{
	this->KillThread();
	
	if (this->RecvFragment)
	{
		delete this->RecvFragment;
		this->RecvFragment = nullptr;
	}
	
	
	std::lock_guard<std::mutex> OGuard { this->OMutex };
	
	while (!this->Outgoing.empty())
	{
		WSMessage *Msg = this->Outgoing.front();
		
		delete Msg;
		this->Outgoing.pop();
	}
	
}

void WS::WSConnection::Send(WSMessage *Msg)
{
	std::lock_guard<std::mutex> OGuard { this->OMutex };
	
	this->Outgoing.push(Msg);
}

WS::WSConnection::~WSConnection(void)
{
	this->Shutdown();
	lws_context_destroy(this->Ctx);
}

WS::WSConnection::WSConnection(void (*const OnNeedPing)(WS::WSConnection *Conn), bool (*const OnReceiveCallback)(void*, WSMessage*), void *UserData)
	:
	OnReceiveCallback(OnReceiveCallback),
	OnReceiveCallbackUserData(UserData),
	OnNeedPing(OnNeedPing),
	Ctx(),
	Socket(),
	Thread(),
	RecvFragment(),
	ShouldDie(),
	ErrorDetected()
{
}
