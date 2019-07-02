#include <libwebsockets.h>
#include <arpa/inet.h>
#include "include/internal/native_ws.h"
#include "include/internal/common.h"

#include <sys/syscall.h>
#include <sys/types.h>

#define GETTID() syscall(SYS_gettid)

/**
 * By the time you are done with this code, you will have developed a deep, seething hatred for WebSockets, libwebsockets, and forced asynchronous network programming.
 **/
 
#define PRE_WS_START LWS_SEND_BUFFER_PRE_PADDING
#define PRE_WS_END LWS_SEND_BUFFER_POST_PADDING
#define PRE_WS_SIZE (PRE_WS_START + PRE_WS_END)

//Prototypes

//Globals
const struct lws_protocols WS::WSConnection::WSProtocols[] = { { "", WS::WSConnection::WSCallback }, { } };

//Implementations

void WS::WSConnection::ThreadFunc(void)
{
	this->ClearError();
	
	this->InitWebSockets();
	
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
	
	if (!this->Socket)
	{
		this->ErrorDetected = true;
		return;
	}
	
	while (!this->ShouldDie)
	{
		lws_service(this->Ctx, 1);
		
		std::unique_lock<std::mutex> G { this->OMutex };
		
		if (!this->Outgoing.empty())
		{
			G.unlock();
			lws_callback_on_writable_all_protocol(this->Ctx, WSProtocols);
		}

	}
}


bool WS::WSConnection::InitWebSockets(void)
{
	if (this->Ctx) lws_context_destroy(this->Ctx);

	struct lws_context_creation_info CtxInfo{};

	CtxInfo.port = CONTEXT_PORT_NO_LISTEN;
	CtxInfo.protocols = WSProtocols;
	CtxInfo.gid = -1;
	CtxInfo.uid = -1;
	
	this->Ctx = lws_create_context(&CtxInfo);
	
	lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, nullptr);
	
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
			lws_callback_on_writable(WSDesc);
			break;
		case LWS_CALLBACK_RECEIVE:
		case LWS_CALLBACK_CLIENT_RECEIVE:
		{
			ThisPtr->AddFragment(Data, DataLength);
			
			lws_callback_on_writable(WSDesc);

			break;
		}
		case LWS_CALLBACK_CLIENT_WRITEABLE:
		{
			std::lock_guard<std::mutex> Guard { ThisPtr->OMutex };
			
			if (ThisPtr->Outgoing.empty())
			{
				break; //Nothing to do.
			}
			
			WSMessage *Msg = ThisPtr->Outgoing.front();
			
			const int MsgSize = Msg->GetRemainingSize() + sizeof(uint32_t);
			int Written = 0, TotalWritten = 0;
			
			uint8_t *DataHead = Msg->GetSeekedBody() - sizeof(uint32_t);
			
			do
			{
				Written = lws_write(WSDesc, DataHead, MsgSize - TotalWritten, LWS_WRITE_BINARY);

				if (Written <= 0) continue;

				TotalWritten += Written;
				DataHead += Written;
			} while (MsgSize > TotalWritten && Written != -1);
			
			
			if (Written != -1 && TotalWritten < MsgSize)
			{ //Only partially transmitted
				Msg->RawSeekForward(TotalWritten);
				break;
			}
			
			//Try again later
			if (Written == -1) break;
			
			//We made it.
			ThisPtr->Outgoing.pop();
			delete Msg;
		}
		case LWS_CALLBACK_CLIENT_CLOSED:
		{
			ThisPtr->Socket = nullptr;
			break;
		}
		case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		{
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

		std::lock_guard<std::mutex> Guard { this->IMutex };
		
		const bool CanKeep = this->OnReceiveCallback(this->OnReceiveCallbackUserData, Msg);
		
		
		if (CanKeep) this->Incoming.push(Msg); //The ticket system doesn't want this message. Probably doesn't have a MsgID.
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
	
	std::lock_guard<std::mutex> IGuard { this->IMutex };
	
	while (!this->Incoming.empty())
	{
		WSMessage *Msg = this->Incoming.front();
		
		delete Msg;
		this->Incoming.pop();
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

WSMessage *WS::WSConnection::Recv(void)
{
	std::lock_guard<std::mutex> IGuard { this->IMutex };
	
	if (this->Incoming.empty()) return nullptr;
	
	WSMessage *RetVal = this->Incoming.front();
	
	this->Incoming.pop();
	
	return RetVal;
}

WS::WSConnection::~WSConnection(void)
{
	this->Shutdown();
	lws_context_destroy(this->Ctx);
}

WS::WSConnection::WSConnection(bool (*const OnReceiveCallback)(void*, WSMessage*), void *UserData)
	:
	OnReceiveCallback(OnReceiveCallback),
	OnReceiveCallbackUserData(UserData),
	Ctx(),
	Socket(),
	Thread(),
	RecvFragment(),
	ShouldDie()
{
}
