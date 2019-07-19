//We have such sights to show you.
#include "include/internal/wsmessages/wsmessages.hpp"
#include "include/internal/common.h"
#include "include/internal/asynctosync.h"
#include "include/internal/msgpackproc.h"
#include "include/internal/native_ws.h"
#include "include/internal/asyncmsgs.h"

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
	
	
	
