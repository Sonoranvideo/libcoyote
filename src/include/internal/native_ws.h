#ifndef __LIBCOYOTE_NATIVE_WS_H__
#define __LIBCOYOTE_NATIVE_WS_H__

#include "common.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <libwebsockets.h>

#include "wsmessages/wsmessages.hpp"

#define EYEBLEED_NOW_MS() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())

namespace WS
{
	static constexpr uint16_t PortNum = 4488;
	static constexpr uint32_t PingInterval = 1000;
	static constexpr uint32_t PingoutMS = 3000;

	class WSConnection
	{ //This class's interface MUST match its webassembly counterpart!
	private:
		static const struct lws_protocols WSProtocols[];

		bool (*OnReceiveCallback)(void*, WSMessage*);
		void *OnReceiveCallbackUserData;

		void (*OnNeedPing)(WSConnection *);
		

		//Instance data members
		std::queue<WSMessage*> Outgoing;
		
		std::mutex OMutex;
		std::string Host;
		
		struct lws_context *Ctx;
		struct lws *Socket;
		
		std::thread *Thread;
		
		std::atomic_uint64_t LastPingMS;
		
		WSMessage::Fragment *RecvFragment;
		std::atomic_bool ShouldDie;
		std::atomic_bool ErrorDetected;
		
		//Private methods
		bool InitWebSockets(void);
		bool InitThreadClient(void);
		static int WSCallback(struct lws *WSDesc, const enum lws_callback_reasons Reason, void *UserData, void *Data, const size_t DataLength);
		void AddFragment(const void *Data, const size_t DataSize);
		
		inline bool AwaitingChunks(void) const { return this->RecvFragment; }
		inline void ClearError(void) { this->ErrorDetected = false; }
		void ThreadFunc(void);
		
		void KillThread(void);
	public:
		WSConnection(void (*const OnNeedPing)(WS::WSConnection *Conn), bool (*const OnReceiveCallback)(void*, WSMessage*), void *UserData);
		~WSConnection(void);
		void Send(WSMessage *Msg);
		bool Connect(const std::string &Host);
		void Shutdown(void);
		inline void RegisterActivity(void) { this->LastPingMS = EYEBLEED_NOW_MS(); }
		bool CheckPingout(void) const;
		bool NeedsPing(void) const;
		inline bool HasError(void) const { return this->ErrorDetected; }
		
		//No copying or moving
		WSConnection(const WSConnection &) = delete;
		WSConnection &operator=(const WSConnection &) = delete;
	};
}

#endif //__LIBCOYOTE_NATIVE_WS_H__
