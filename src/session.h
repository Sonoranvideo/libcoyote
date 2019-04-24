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

#ifndef __LIBCOYOTE_SESSION_H__
#define __LIBCOYOTE_SESSION_H__

#ifndef __cplusplus
#error "This is the C++ set of bindings, you're looking for libcoyote_c.h"
#endif //__cplusplus
#include <string>
#include <stdint.h>

namespace Coyote
{
	class Session
	{
	private:
		void *Internal;
	public:
		Session(const std::string &URL, const uint16_t PortNum = 8000);
		Session(Session &&In);
		Session &operator=(Session &&In);
		
		//Disallow copying
		Session(const Session &) = delete;
		Session &operator=(const Session &) = delete;
		
		StatusCode GetAssets(std::vector<Coyote::Asset> &Out);
		StatusCode GetPresets(std::vector<Coyote::Preset> &Out);
		
		virtual ~Session(void);
	};
	
}
#endif //__LIBCOYOTE_SESSION_H__
