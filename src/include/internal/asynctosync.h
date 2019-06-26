/*Where we're going, we don't need eyes to see.*/

#ifndef __LIBCOYOTE_HORROR_H__
#define __LIBCOYOTE_HORROR_H__


#define MSGPACK_DEFAULT_API_VERSION 2

#include <msgpack.hpp>
#include <mutex>
#include <condition_variable>

#include "native_ws.h"

namespace AsyncToSync
{
	template <typename T = void*>
	class MTEvent
	{
	private:
		std::condition_variable Var;
		mutable std::mutex Mutex;
		T Lump;
		
	public:
		MTEvent(void) : Lump() {}
		
		T Wait(void) //Return by value!
		{
			std::unique_lock<std::mutex> Lock { this->Mutex };
			
			this->Var.wait(Lock);
			
			return this->Lump;
		}
		
		void Post(const T &Value = {})
		{
			const std::lock_guard<std::mutex> Lock { this->Mutex };
		
			this->Lump = Value;
			
			this->Var.notify_all();
		}
		
		const T *Peek(void) const
		{
			const std::lock_guard<std::mutex> Lock { this->Mutex };
			
			return &this->Lump;
		}
	};
	
	class MessageTicket
	{
	private:
		MTEvent<WS::WSMessage*> Event;		
		uint64_t MsgID;

	public:
		inline WS::WSMessage *WaitForRecv(void)
		{
			return this->Event.Wait();
		}
		
		inline void SetReady(WS::WSMessage *Msg)
		{
			this->Event.Post(Msg);
		}

		inline MessageTicket(const uint64_t MsgID) : Event(), MsgID(MsgID)
		{
		}
		
		inline ~MessageTicket(void)
		{
			if (*this->Event.Peek()) delete *this->Event.Peek();
		}
		
		inline uint64_t GetMsgID(void) const { return this->MsgID; }
		
		//No copying
		MessageTicket(const MessageTicket &) = delete;
		MessageTicket &operator=(const MessageTicket &) = delete;
	};
	
	class SynchronousSession
	{
	private:
		std::map<uint64_t, MessageTicket*> Tickets;
		std::mutex TicketsLock;
		
	public:
		~SynchronousSession(void)
		{
			std::lock_guard<std::mutex> G { this->TicketsLock };
			
			for (auto Iter = this->Tickets.begin(); Iter != this->Tickets.end(); ++Iter)
			{
				delete Iter->second;
			}
		}
		
		static bool OnMessageReady(void *ThisPtr_, WS::WSMessage *Msg);
		MessageTicket *NewTicket(const uint64_t MsgID);
		
		
		//No copying/*
		SynchronousSession(const SynchronousSession &) = delete;
		SynchronousSession &operator=(const SynchronousSession &) = delete;
		SynchronousSession(void) = default;
	};
	
}
#endif //__LIBCOYOTE_HORROR_H__
