/*Where we're going, we don't need eyes to see.*/

#ifndef __LIBCOYOTE_HORROR_H__
#define __LIBCOYOTE_HORROR_H__


#define MSGPACK_DEFAULT_API_VERSION 2

#include <msgpack.hpp>
#include <mutex>
#include <pthread.h>
#include <semaphore.h>
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
		
		bool Wait(T &ValueOut, const time_t Timeout = 1) //Return by value!
		{
			bool Success = false;
			
			for (uint32_t Inc = 0; Inc < Timeout * 1000 && !Success; ++Inc)
			{
				Success = sem_trywait(&this->Semaphore) == 0;
				
				COYOTE_SLEEP(Timeout);
			}
				
			
			if (!Success)
			{ //We timed out.
				return false;
			}
			
			std::unique_lock<std::mutex> Lock { this->Mutex };
			
			ValueOut = std::move(this->Lump);
			
			this->Lump = {};
			
			return true;
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
			WSMessage *RetVal = nullptr;
			
			if (!this->Event.Wait(RetVal)) return nullptr;
			
			return RetVal;
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
		}
		
		inline uint64_t GetMsgID(void) const { return this->MsgID; }
		
		//No copying
		MessageTicket(const MessageTicket &) = delete;
		MessageTicket &operator=(const MessageTicket &) = delete;
	};
	
	class MsgIDCounter
	{
	private:
		std::mutex Lock;
		uint64_t Value;
	public:
		MsgIDCounter(void) : Value(1) {}
		
		uint64_t NewID(void)
		{
			const std::lock_guard<std::mutex> Guard { this->Lock };
			
			const uint64_t Value = this->Value++;
			
			return Value;
		}
	};
	
	class SynchronousSession
	{
	private:
		std::map<uint64_t, MessageTicket*> Tickets;
		std::mutex TicketsLock;
		MsgIDCounter MsgIDs;
		
	public:
		~SynchronousSession(void)
		{
			std::lock_guard<std::mutex> G { this->TicketsLock };
			
			for (auto Iter = this->Tickets.begin(); Iter != this->Tickets.end(); ++Iter)
			{
				delete Iter->second;
			}
		}
		
		bool OnMessageReady(const std::map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg);
		MessageTicket *NewTicket(const uint64_t MsgID);
		bool DestroyTicket(MessageTicket *Ticket);
		uint64_t NewMsgID(void) { return this->MsgIDs.NewID(); }
		
		//No copying/*
		SynchronousSession(const SynchronousSession &) = delete;
		SynchronousSession &operator=(const SynchronousSession &) = delete;
		SynchronousSession(void) = default;
	};
	
}
#endif //__LIBCOYOTE_HORROR_H__
