//We have such sights to show you.

#include "include/internal/common.h"
#include "include/internal/asynctosync.h"
#include "include/internal/msgpackproc.h"
#include "include/internal/native_ws.h"

bool AsyncToSync::SynchronousSession::OnMessageReady(void *ThisPtr_, WSMessage *Msg)
{
	SynchronousSession *ThisPtr = static_cast<SynchronousSession*>(ThisPtr_);
	
	//This is horrifically inefficient but not enough so to really affect our throughput in a meaningful way.
	std::map<std::string, msgpack::object> Values;
	
	try
	{
		Values = MsgpackProc::InitIncomingMsg(Msg->GetBody(), Msg->GetBodySize());
	}
	catch(...)
	{
		std::cout << "Garbage data detected. Discarded.\n";
		return false;
	}
	
	if (!Values.count("MsgID")) return true; //WSConnection can keep it.
	
	const uint64_t MsgID = Values.at("MsgID").as<uint64_t>();
	
	std::lock_guard<std::mutex> Guard { ThisPtr->TicketsLock };
	
	if (!ThisPtr->Tickets.count(MsgID)) //Unclaimed, likely asynchronous message. Let WSConnection collect it.
	{
		return true;
	}

	MessageTicket *Ticket = ThisPtr->Tickets.at(MsgID);
	
	assert(MsgID == Ticket->GetMsgID());
	
	Ticket->SetReady(Msg);
	
	ThisPtr->Tickets.erase(MsgID); //Remove it from our queue, but don't deallocate it, because someone else might be waiting on it. That's their responsibility.
	
	return false;
}

AsyncToSync::MessageTicket *AsyncToSync::SynchronousSession::NewTicket(const uint64_t MsgID)
{
	assert(MsgID != 0);
	
	std::lock_guard<std::mutex> G { this->TicketsLock };
	
	if (this->Tickets.count(MsgID)) return nullptr; //Wtf happened here
	
	return (this->Tickets[MsgID] = new MessageTicket(MsgID));
}
