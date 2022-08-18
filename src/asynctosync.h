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

/*Where we're going, we don't need eyes to see.*/

#ifndef __LIBCOYOTE_HORROR_H__
#define __LIBCOYOTE_HORROR_H__


#define MSGPACK_DEFAULT_API_VERSION 2

#include "msgpack.hpp"
#include <mutex>
#include <condition_variable>
#include "include/common.h"
#include "native_ws.h"
#include "mtevent.h"

namespace AsyncToSync
{
	
	
	class MessageTicket
	{
	private:
		MTEvent<WSMessage*> Event;		
		uint64_t MsgID;

	public:
		inline WSMessage *WaitForRecv(const time_t TimeoutSecs = 1)
		{
			WSMessage *RetVal = nullptr;
			
			if (!this->Event.Wait(RetVal, TimeoutSecs)) return nullptr;
			
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
		
		inline void TriggerDeath(void) { this->Event.TriggerDeath(); }
		
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
				Iter->second->TriggerDeath();
			}
			
			this->Tickets.clear();
		}
		
		bool OnMessageReady(const std::map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg);
		MessageTicket *NewTicket(const uint64_t MsgID);
		bool DestroyTicket(MessageTicket *Ticket);
		uint64_t NewMsgID(void) { return this->MsgIDs.NewID(); }
		void DestroyAllTickets(void);
		//No copying/*
		SynchronousSession(const SynchronousSession &) = delete;
		SynchronousSession &operator=(const SynchronousSession &) = delete;
		SynchronousSession(void) = default;
	};
	
}
#endif //__LIBCOYOTE_HORROR_H__
