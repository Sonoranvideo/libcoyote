#ifndef __LIBCOYOTE_ASYNCMSGS_H__
#define __LIBCOYOTE_ASYNCMSGS_H__

#include "common.h"
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
		
	};
}
#endif //__LIBCOYOTE_ASYNCMSGS_H__
