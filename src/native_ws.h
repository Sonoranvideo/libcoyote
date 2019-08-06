#ifndef __LIBCOYOTE_NATIVE_WS_H__
#define __LIBCOYOTE_NATIVE_WS_H__

#include "../wsmessages/wsmessages.hpp"

#include "common.h"
#include "mtevent.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <QtCore>
#include <QtWebSockets>


#define EYEBLEED_NOW_MS() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())

namespace WS
{
	static constexpr uint16_t PortNum = 4488;
	static constexpr uint32_t PingInterval = 1000;
	static constexpr uint32_t PingoutMS = 3000;
	
	class WSCore;
	
	class WSConnection : public QObject
	{ //This class's interface MUST match its webassembly counterpart!
		Q_OBJECT
	private:
		//Instance data members
		std::queue<WSMessage*> Outgoing;
		
		std::mutex OMutex;
		std::string Host;
		
		bool (*OnReceiveCallback)(WSConnection*, WSMessage*);

		std::unique_ptr<WSMessage::Fragment> RecvFragment;

		std::unique_ptr<QWebSocket> WebSocket;		
		std::atomic_uint64_t LastPingMS;
		std::atomic_bool ErrorDetected;
		
		//Private methods
		WSMessage *AddFragment(const void *Data, const size_t DataSize);
		
		inline bool AwaitingChunks(void) const { return this->RecvFragment.get(); }
		inline void ClearError(void) { this->ErrorDetected = false; }
		
		bool EstablishConnection(const std::string &Host);
	public:
		WSConnection(bool (*const OnReceiveCallback)(WSConnection*, WSMessage*), void *UserData = nullptr);
		virtual ~WSConnection(void);
		void Send(WSMessage *Msg);
		void Shutdown(void);
		inline void RegisterActivity(void) { this->LastPingMS = EYEBLEED_NOW_MS(); }
		bool CheckPingout(void) const;
		bool NeedsPing(void) const;
		void SendPing(void);
		inline bool HasError(void) const { return this->ErrorDetected; }
		void ProcessOutgoingMsgs(void);

		//No copying or moving
		WSConnection(const WSConnection &) = delete;
		WSConnection( WSConnection &&) = delete;
		WSConnection &operator=(const WSConnection &) = delete;
		WSConnection &operator=(WSConnection &&) = delete;
		
		void *UserData;

		friend class WSCore;
	public slots:
		void OnRecv(const QByteArray &Data);
		void OnConnected(void);
	};
	
	
	class WSCore : public QObject
	{
		Q_OBJECT
	private:
		struct ConnStruct 
		{
			std::string URI;
			void *UserData;
			WSConnection *Out;
		};
		
		static std::atomic<WS::WSCore *> Instance;
		static std::atomic<std::thread *> Thread;

		bool (*RecvCallback)(WSConnection*, WSMessage*);
		
		static QCoreApplication *AppObject;
		
		QEventLoop *EventLoop;

		std::mutex ConnectionQueueLock;
		std::mutex ConnectionsLock;
		
		std::queue<MTEvent<ConnStruct> *> ConnectionQueue;
		std::vector<WSConnection*> Connections;
		
		void MasterThread(void);		
		void ProcessNewConnections(void);
		void ProcessAllOutgoing(void);
		void CheckConnections(void);
		WSCore(WSCore &&) = delete;
		WSCore(const WSCore &) = delete;
		WSCore & operator=(const WSCore &) = delete;
		WSCore & operator=(WSCore &&) = delete;
		static void InitThread(bool (*const OnReceiveCallback)(WSConnection*, WSMessage*));
	public:
		static inline WSCore *GetInstance(void) { return WSCore::Instance; }
		static void Fireup(bool (*const OnReceiveCallback)(WSConnection*, WSMessage*));
		WSCore(bool (*const OnReceiveCallback)(WSConnection*, WSMessage*));
		virtual ~WSCore(void);


		WSConnection *NewConnection(const std::string &Host, void *UserData = nullptr);
	public slots:
		void MainLoopBody(void);
		
	};
}

#endif //__LIBCOYOTE_NATIVE_WS_H__
