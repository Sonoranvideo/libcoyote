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

#define WSMESSAGES_NOLWS
#define MSGPACK_DEFAULT_API_VERSION 2
#include "msgpack.hpp"

#include "include/common.h"
#include "../wsmessages/wsmessages.hpp"
#include "asyncmsgs.h"
#include "msgpackproc.h"
#include "subscriptions.h"

bool AsyncMsgs::AsynchronousSession::OnMessageReady(const std::unordered_map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg_)
{
	const std::unique_ptr<WSMessage> Msg { Msg_ };
	
	this->SubSession.ProcessSubscriptionEvent(Values);
	
	return true;
}

