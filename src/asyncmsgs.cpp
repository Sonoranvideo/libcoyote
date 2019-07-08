#include "include/internal/asyncmsgs.h"

bool AsyncMsgs::AsynchronousSession::OnMessageReady(const std::map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg)
{
	return true;
}

