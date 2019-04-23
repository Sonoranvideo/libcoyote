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

#ifndef __LIBCOYOTE_CURLREQUESTS_H__
#define __LIBCOYOTE_CURLREQUESTS_H__

#include "common.h"

namespace CurlRequests
{
	enum CurlOperation : uint8_t
	{
		OPERATION_INVALID = 0,
		OPERATION_GET,
		OPERATION_POST,
		OPERATION_PUT,
		OPERATION_MAX
	};

	class CurlSession
	{
	private:
		static bool InitPerformed;
		
		void *CurlDescriptor;
		
		static size_t CurlWriteFunc(const char *Ptr, size_t UnitSize, size_t NumUnits, std::vector<uint8_t> *Response);

	public:
		CurlSession(void);
		~CurlSession(void);
		
		bool SendJSON(const std::string &URL, const std::string &JSONData, std::vector<uint8_t> *Response = nullptr);
	};
}

#endif //__LIBCOYOTE_CURLREQUESTS_H__
