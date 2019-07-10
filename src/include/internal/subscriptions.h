#ifndef __LIBCOYOTE_SUBSCRIPTIONS_H__
#define __LIBCOYOTE_SUBSCRIPTIONS_H__

#include "common.h"
#include "../datastructures.h"
#define MSGPACK_DEFAULT_API_VERSION 2
#include <msgpack.hpp>
#include <mutex>

namespace Subs
{
	class SubscriptionSession
	{
	private:
		std::mutex CurTimeCodeLock;
		Coyote::TimeCode CurTimeCode;
	public:
		bool ProcessSubscriptionEvent(const std::map<std::string, msgpack::object> &Values);
		Coyote::TimeCode *GetTimeCode(void);
		SubscriptionSession() : CurTimeCode() { }
	};
}
#endif //__LIBCOYOTE_SUBSCRIPTIONS_H__
