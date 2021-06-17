/*
   Copyright 2021 Sonoran Video Systems

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

#ifndef __LIBCOYOTE_ASYNCMSGS_H__
#define __LIBCOYOTE_ASYNCMSGS_H__

#include "include/common.h"
#include "native_ws.h"
#include "msgpackproc.h"
#include "subscriptions.h"

namespace AsyncMsgs
{
	class AsynchronousSession
	{
	private:
		
	public:
		bool OnMessageReady(const std::map<std::string, msgpack::object> &Values, WS::WSConnection *Conn, WSMessage *Msg);
		
		Subs::SubscriptionSession SubSession;
		
		AsynchronousSession(void) {}
		
		inline void SetStateEventCallback(const Coyote::StateEventType EType, const Coyote::StateEventCallback CB, void *const UserData)
		{
			SubSession.SetStateEventCallback(EType, CB, UserData);
		}
		inline void SetPlaybackEventCallback(const Coyote::PBEventCallback CB, void *const UserData)
		{
			SubSession.SetPlaybackEventCallback(CB, UserData);
		}

		
	};
}
#endif //__LIBCOYOTE_ASYNCMSGS_H__
