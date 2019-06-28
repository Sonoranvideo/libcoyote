#ifndef __LIBCOYOTE_NATIVE_WS_H__
#define __LIBCOYOTE_NATIVE_WS_H__

#include "common.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <libwebsockets.h>

#include "wsmessages/wsmessages.hpp"

namespace WS
{
	static constexpr uint16_t PortNum = 4488;
	
	class WSConnection
	{ //This class's interface MUST match its webassembly counterpart!
	private:
		static const struct lws_protocols WSProtocols[];

		bool (*OnReceiveCallback)(void*, WSMessage*);
		void *OnReceiveCallbackUserData;

		//Instance data members
		std::queue<WSMessage*> Incoming;
		std::queue<WSMessage*> Outgoing;
		
		std::mutex IMutex, OMutex;
		
		struct lws_context *Ctx;
		struct lws *Socket;
		
		std::thread *Thread;
		
		WSMessage::Fragment *RecvFragment;
		std::atomic_bool ShouldDie;
		
		//Private methods
		bool InitWebSockets(void);
		static int WSCallback(struct lws *WSDesc, const enum lws_callback_reasons Reason, void *UserData, void *Data, const size_t DataLength);
		void AddFragment(const void *Data, const size_t DataSize);
		
		inline bool AwaitingChunks(void) const { return this->RecvFragment; }
		
		void ThreadFunc(void);
		
		void KillThread(void);
	public:
		WSConnection(bool (*const OnReceiveCallback)(void*, WSMessage*), void *UserData);
		~WSConnection(void);
		void Send(WSMessage *Msg);
		WSMessage *Recv(void);
		bool Connect(const std::string &Host);
		void Shutdown(void);
		
		//No copying or moving
		WSConnection(const WSConnection &) = delete;
		WSConnection &operator=(const WSConnection &) = delete;
	};
}

#endif //__LIBCOYOTE_NATIVE_WS_H__
