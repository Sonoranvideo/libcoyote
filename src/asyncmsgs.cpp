#include "../wsmessages/wsmessages.hpp"
#include "asyncmsgs.h"
#include "msgpackproc.h"
#include "subscriptions.h"

bool AsyncMsgs::AsynchronousSession::OnMessageReady(const std::map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg)
{
	this->SubSession.ProcessSubscriptionEvent(Values);
	
	return true;
}

