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

//We have such sights to show you.
#include "../wsmessages/wsmessages.hpp"
#include "common.h"
#include "asynctosync.h"
#include "msgpackproc.h"
#include "native_ws.h"
#include "asyncmsgs.h"

bool AsyncToSync::SynchronousSession::OnMessageReady(const std::map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg)
{	
	assert(Values.count("MsgID"));
	
	if (!Values.count("MsgID")) return false;
	
	const uint64_t MsgID = Values.at("MsgID").as<uint64_t>();
	
	std::lock_guard<std::mutex> Guard { this->TicketsLock };

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
	std::lock_guard<std::mutex> G { this->TicketsLock };

	const uint64_t MsgID = Ticket->GetMsgID();
	
	if (!this->Tickets.count(MsgID)) return false;

	assert(this->Tickets.at(MsgID) == Ticket);
	
	this->Tickets.erase(MsgID);
	
	delete Ticket;
	
	return true;
}
	
	
	
