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
#ifndef __LIBCOYOTE_SUBSCRIPTIONS_H__
#define __LIBCOYOTE_SUBSCRIPTIONS_H__

#include "common.h"
#include "include/datastructures.h"
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
