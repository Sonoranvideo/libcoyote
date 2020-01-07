/*
   Copyright 2020 Sonoran Video Systems

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

//We have such sights to show you.
#define WSMESSAGES_NOLWS
#include "../wsmessages/wsmessages.hpp"
#include "include/common.h"
#include "asynctosync.h"
#include "msgpackproc.h"
#include "native_ws.h"
#include "asyncmsgs.h"

bool AsyncToSync::SynchronousSession::OnMessageReady(const std::map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg)
{	
	assert(Values.count("MsgID"));
	
	if (!Values.count("MsgID")) return false;
	
	const uint64_t MsgID = Values.at("MsgID").as<uint64_t>();
	
	const std::lock_guard<std::mutex> Guard { this->TicketsLock };

	if (!this->Tickets.count(MsgID)) //Unclaimed, likely asynchronous message. Let WSConnection collect it.
	{
		return true;
	}

	MessageTicket *Ticket = this->Tickets.at(MsgID);
	
	
	assert(MsgID == Ticket->GetMsgID());
	
	Ticket->SetReady(Msg);

	return false;
}

AsyncToSync::MessageTicket *AsyncToSync::SynchronousSession::NewTicket(const uint64_t MsgID)
{
	assert(MsgID != 0);
	
	std::lock_guard<std::mutex> G { this->TicketsLock };
	
	if (this->Tickets.count(MsgID)) return nullptr; //Wtf happened here
	
	return (this->Tickets[MsgID] = new MessageTicket(MsgID));
}

bool AsyncToSync::SynchronousSession::DestroyTicket(MessageTicket *Ticket)
{
	if (!Ticket) return false;
	
	const std::lock_guard<std::mutex> G { this->TicketsLock };

	//Check that we have such a ticket pointer at all before we dereference it. This is costly and mostly just paranoia.
	//We're so deep in the semaphore/mutex crap that I can't keep it straight in my own head, so I'll just be paranoid.
	bool Found = false;
	
	for (auto &Ref : this->Tickets)
	{
		 if (Ref.second == Ticket)
		 {
			 Found = true;
			 break;
		 }
	}
		 
	
	if (!Found)
	{ //Probably orphan
		delete Ticket;
		return false;
	}
	
	const uint64_t MsgID = Ticket->GetMsgID();
	
	if (!this->Tickets.count(MsgID)) return false;

	assert(this->Tickets.at(MsgID) == Ticket);
	
	this->Tickets.erase(MsgID);
	
	delete Ticket;
	
	return true;
}
	
	
void AsyncToSync::SynchronousSession::DestroyAllTickets(void)
{
	const std::lock_guard<std::mutex> G { this->TicketsLock };

	for (auto &Ref : this->Tickets)
	{
		Ref.second->TriggerDeath();
	}
	
	this->Tickets.clear();
}

