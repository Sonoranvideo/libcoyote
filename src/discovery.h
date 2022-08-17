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
#ifndef __LIBCOYOTE_DISCOVERY_H__
#define __LIBCOYOTE_DISCOVERY_H__

#include "include/datastructures.h"
#include <QtNetwork>

namespace Discovery
{
	struct InternalBCastMsg
	{ //Don't put any datatypes bigger than a byte in here, or we can't be lazy. We'd have to deal with alignment.
		char APIVersion[32];
		char CommunicatorVersion[32];
		char GUID[64];
		char Nickname[128];
		uint8_t CurrentRole;
		uint8_t Type;
		
		inline Coyote::LANCoyote ToPublicStruct(const std::string &IP)
		{
			Coyote::LANCoyote R{};
			
			R.APIVersion = APIVersion;
			R.CommunicatorVersion = CommunicatorVersion;
			R.GUID = GUID;
			R.Nickname = Nickname;
			R.CurrentRole = static_cast<Coyote::UnitRole>(CurrentRole);
			R.Type = static_cast<Coyote::UnitType>(Type);
			R.IP = IP;
			return R;
		}
	};
	
	inline std::string NormalizeIPV4(std::string Input)
	{
		const char *const Checker = "::ffff:";
	
		if (!strncmp(Input.c_str(), Checker, sizeof Checker - 1) && strchr(Input.c_str(), '.') != nullptr)
		{ //IPv4 address encoded as IPv6 compatibility, confuses everything else that uses real IPv4 addresses.
			Input = std::string{Input.c_str() + (sizeof Checker - 1)};
		}
	
		return std::move(Input);
	}
	
	class DiscoverySession
	{ //Should only ever be ONE instance of this.
	private:
		std::map<std::string, Coyote::LANCoyote> KnownCoyotes;
		std::mutex KnownCoyotesLock;
		std::unique_ptr<QThread> Thread;
		std::unique_ptr<QUdpSocket> Sock;
		static DiscoverySession *Instance;
		
		void ThreadFunc(void);
		void ProcessBroadcasts(void);
	public:
		DiscoverySession(void);
		
		static inline DiscoverySession *CheckInit(void)
		{
			if (DiscoverySession::Instance) return DiscoverySession::Instance;
			
			return new DiscoverySession;
		}
		
		std::vector<Coyote::LANCoyote> GetLANCoyotes(void);
	};
}

#endif //__LIBCOYOTE_DISCOVERY_H__
