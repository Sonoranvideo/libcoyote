#include "include/internal/wsmessages/wsmessages.hpp"
#include "include/internal/asyncmsgs.h"
#include "include/internal/msgpackproc.h"
#include "include/internal/subscriptions.h"

bool AsyncMsgs::AsynchronousSession::OnMessageReady(const std::map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg)
{
	this->SubSession.ProcessSubscriptionEvent(Values);
	
	return true;
}

