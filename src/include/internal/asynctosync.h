/*Where we're going, we don't need eyes to see.*/

#ifndef __LIBCOYOTE_HORROR_H__
#define __LIBCOYOTE_HORROR_H__


#define MSGPACK_DEFAULT_API_VERSION 2

#include <msgpack.hpp>
#include <mutex>
#include <pthread.h>
#include <condition_variable>

#include "native_ws.h"

namespace AsyncToSync
{
	template <typename T = void*>
	class MTEvent
	{
	private:
		mutable std::mutex Mutex;
		T Lump;
		sem_t Semaphore;
	public:
		MTEvent(void) : Lump(), Semaphore()
		{
			sem_init(&this->Semaphore, 0, 0);
		}
		
		~MTEvent(void)
		{
			sem_destroy(&this->Semaphore);
		}
		
		T Wait(void) //Return by value!
		{
			while (sem_wait(&this->Semaphore));
			
			std::unique_lock<std::mutex> Lock { this->Mutex };
			
			return this->Lump;
		}
		
		void Post(const T &Value = {})
		{
			const std::lock_guard<std::mutex> Lock { this->Mutex };
		
			this->Lump = Value;
			
			sem_post(&this->Semaphore);
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
		MTEvent<WSMessage*> Event;		
		uint64_t MsgID;

	public:
		inline WSMessage *WaitForRecv(void)
		{
			return this->Event.Wait();
		}
		
		inline void SetReady(WSMessage *Msg)
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
		
		static bool OnMessageReady(void *ThisPtr_, WSMessage *Msg);
		MessageTicket *NewTicket(const uint64_t MsgID);
		
		
		//No copying/*
		SynchronousSession(const SynchronousSession &) = delete;
		SynchronousSession &operator=(const SynchronousSession &) = delete;
		SynchronousSession(void) = default;
	};
	
}
#endif //__LIBCOYOTE_HORROR_H__
