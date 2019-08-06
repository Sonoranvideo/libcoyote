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
#include <QtWebSockets>
#include <QCoreApplication>
#include <moc_native_ws.cpp>
#include "common.h"
#include "msgpackproc.h"
#include <msgpack.hpp>
#define qs2cs(s) (s.toUtf8().constData()) //Irritating enough to type without caps

/**
 * By the time you are done with this code, you will have developed a deep, seething hatred for WebSockets, libwebsockets, and forced asynchronous network programming.
 **/
 
#define PRE_WS_START LWS_SEND_BUFFER_PRE_PADDING
#define PRE_WS_END LWS_SEND_BUFFER_POST_PADDING
#define PRE_WS_SIZE (PRE_WS_START + PRE_WS_END)

//Fake argv/argc for Qt
static int argc = 1;

static char **argv1 = new char* { (char*)"libcoyote" };

//Prototypes
void SessionSneak_DeactivateConnection(void *Ptr); //So we don't need to include session.h

//Globals
std::atomic<WS::WSCore *> WS::WSCore::Instance;
std::atomic<std::thread *> WS::WSCore::Thread;
QCoreApplication *WS::WSCore::AppObject;

//Implementations

WS::WSCore::WSCore(bool (*const OnReceiveCallback)(WSConnection*, WSMessage*))
	: RecvCallback(OnReceiveCallback) //Might be null
{
}

void WS::WSConnection::SendPing(void)
{
	msgpack::sbuffer Buffer;
	msgpack::packer<msgpack::sbuffer> Pack { Buffer };
	
	MsgpackProc::InitOutgoingMsg(Pack, "Ping");
	
	this->Send(new WSMessage(Buffer.data(), Buffer.size()));
}

void WS::WSConnection::ProcessOutgoingMsgs(void)
{
	std::unique_lock<std::mutex> OGuard { this->OMutex };

	while (!this->Outgoing.empty())
	{
		WSMessage *Msg = this->Outgoing.front();
		
		OGuard.unlock();
		
		const int MsgSize = Msg->GetRemainingSize() + sizeof(uint32_t);
		int Written = 0, TotalWritten = 0;
		
		uint8_t *DataHead = Msg->GetSeekedBody() - sizeof(uint32_t);
		do
		{
			try
			{
				Written = this->WebSocket->sendBinaryMessage(QByteArray((const char*)DataHead, MsgSize - TotalWritten));
			}
			catch(...)
			{
				Written = -1;
			}
			
			if (Written <= 0) continue;
			
			this->RegisterActivity(); //Write call didn't fail, so we can count this as a win.
			
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
			this->ErrorDetected = true;
			break;
		}
		
		OGuard.lock();
		//We made it.
		this->Outgoing.pop();
		delete Msg;
	}
}
void WS::WSCore::ProcessAllOutgoing(void)
{
	std::unique_lock<std::mutex> Guard { this->ConnectionsLock };
	
	for (WSConnection *Connection : this->Connections)
	{
		if (Connection->HasError()) continue;
		
		Connection->ProcessOutgoingMsgs();
	}
}

void WS::WSCore::MainLoopBody(void)
{
	this->ProcessNewConnections();
	this->ProcessAllOutgoing();
	this->CheckConnections();
}


void WS::WSCore::CheckConnections(void)
{
	const std::lock_guard<std::mutex> Guard { this->ConnectionsLock };
	
Rescan:
	for (auto Iter = this->Connections.begin(); Iter != this->Connections.end(); ++Iter)
	{
		WSConnection *Connection = *Iter;
		
		if (Connection->HasError() || Connection->CheckPingout())
		{
			std::cout << "LibCoyote: Detected dead connection, pruning" << std::endl;
			
			SessionSneak_DeactivateConnection(Connection->UserData);
			
			this->Connections.erase(Iter);
			goto Rescan;
		}
		
		if (Connection->NeedsPing()) Connection->SendPing();
	}
}

void WS::WSCore::ProcessNewConnections(void)
{
	const std::lock_guard<std::mutex> Guard { this->ConnectionQueueLock };
	
	while (!this->ConnectionQueue.empty())
	{
		MTEvent<ConnStruct> *Event = this->ConnectionQueue.front();
		
		const ConnStruct *Struct = Event->Peek();
		
		WSConnection *Conn = new WSConnection(this->RecvCallback, Struct->UserData);
		Conn->EstablishConnection(Struct->URI);
		
		std::unique_lock<std::mutex> G { this->ConnectionsLock };
		
		this->Connections.push_back(Conn);

		Event->Post(ConnStruct { Struct->URI, Struct->UserData, Conn });
		
		this->ConnectionQueue.pop();
	}
}

void WS::WSCore::InitThread(bool (*const OnReceiveCallback)(WSConnection*, WSMessage*))
{
	WSCore::Instance = new WSCore { OnReceiveCallback };
	WSCore *Ptr = Instance;
	
	Ptr->MasterThread();
}

void WS::WSCore::MasterThread(void)
{
	this->EventLoop = new QEventLoop;

	QTimer *LoopTimer = new QTimer;
	
	LoopTimer->connect(LoopTimer, &QTimer::timeout, this, &WSCore::MainLoopBody);
	LoopTimer->start(10);
	
	
	this->EventLoop->exec();
}

WS::WSConnection *WS::WSCore::NewConnection(const std::string &Host, void *UserData)
{ //Called by the user side.
	WSCore::ConnStruct Val{ Host, UserData };
	
	std::unique_lock<std::mutex> Guard { this->ConnectionQueueLock };
	
	MTEvent<WSCore::ConnStruct> *Event { new MTEvent<WSCore::ConnStruct> { &Val } };
	
	this->ConnectionQueue.push(Event);
	
	Guard.unlock();
	
	while (!Event->Wait(Val, 10));
	
	delete Event;
	
	return Val.Out;
}

void WS::WSConnection::OnConnected(void)
{
	this->RegisterActivity();
	this->ProcessOutgoingMsgs();
}

void WS::WSConnection::OnRecv(const QByteArray &Data)
{
	this->RegisterActivity();
	
	//Yay, an excuse to send outgoing messages.
	this->ProcessOutgoingMsgs();
	
	WSMessage *Msg = this->AddFragment(Data.data(), Data.size());
	
	if (!Msg) return;
	
	this->OnReceiveCallback(this, Msg);
}

bool WS::WSConnection::EstablishConnection(const std::string &Host)
{
	this->Host = Host;
	
	this->ClearError();

	this->WebSocket.reset(new QWebSocket);
	
	const QString URL { QString("ws://") + this->Host.c_str() + ":" + QString::number(WS::PortNum) + "/" };
	
	//Set the error flag if we lose our connection.
	this->WebSocket->connect(this->WebSocket.get(), QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), [this] (QAbstractSocket::SocketError) { this->ErrorDetected = true; });
	
	try
	{
		this->WebSocket->open(URL);
	}
	catch (const QAbstractSocket::SocketError &ErrType)
	{
		this->ErrorDetected = true;
		return false;
	}
	
	//Bring up the socket.
	
	this->connect(this->WebSocket.get(), &QWebSocket::binaryMessageReceived, this, &WSConnection::OnRecv);
	
	QEventLoop Waiter;
	
	this->connect(this->WebSocket.get(), &QWebSocket::connected, this,  [this, &Waiter] { this->OnConnected(); Waiter.quit(); });

	std::cout << "LibCoyote: Attempting to establish a new connection to " << qs2cs(URL) << std::endl;
	Waiter.exec();
	
	std::cout << "LibCoyote: Connection established." << std::endl;
	
	return true;
}

bool WS::WSConnection::CheckPingout(void) const
{ //True if we're dead
	return EYEBLEED_NOW_MS() - this->LastPingMS > PingInterval + PingoutMS;
}

bool WS::WSConnection::NeedsPing(void) const
{
	return EYEBLEED_NOW_MS() - this->LastPingMS > PingInterval;
}

WSMessage *WS::WSConnection::AddFragment(const void *Data, const size_t DataSize)
{
	if (!this->RecvFragment)
	{
		this->RecvFragment.reset(new WSMessage::Fragment(Data, DataSize));
	}
	else
	{
		this->RecvFragment->Append(Data, DataSize);
	}
	
	if (this->RecvFragment->IsComplete())
	{
		WSMessage *Msg = this->RecvFragment->Graduate();
		this->RecvFragment.reset();

		return Msg;
	}
	
	return nullptr;
}

void WS::WSConnection::Shutdown(void)
{
	if (this->WebSocket)
	{
		this->WebSocket->close();
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
	this->WebSocket->close();
}

WS::WSConnection::WSConnection(bool (*const OnReceiveCallback)(WSConnection*, WSMessage*), void *UserData)
	:
	OnReceiveCallback(OnReceiveCallback),
	RecvFragment(),
	LastPingMS(),
	ErrorDetected(),
	UserData(UserData)
{
}

WS::WSCore::~WSCore(void)
{
	assert(!"This WSCore object should never be destroyed!");
}

void WS::WSCore::Fireup(bool (*const OnReceiveCallback)(WSConnection*, WSMessage*))
{
	
	WSCore::AppObject = QCoreApplication::instance() ? QCoreApplication::instance() : new QCoreApplication(argc, argv1);
	
	WS::WSCore::Thread = new std::thread(&WSCore::InitThread, OnReceiveCallback);
	
	while (!WSCore::Instance)
	{
		COYOTE_SLEEP(100);
	}
}
