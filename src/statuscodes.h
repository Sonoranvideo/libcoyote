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

#ifndef __LIBCOYOTE_STATUSCODES_H__
#define __LIBCOYOTE_STATUSCODES_H__

#include "common.h"

namespace Coyote
{
	enum StatusCode : int8_t
	{
		STATUS_INVALID 				= -1,
		STATUS_OK 					= 0,
		STATUS_FAILED 				= 1,
		STATUS_UNIMPLEMENTED 		= 2,
		STATUS_INTERNALERROR 		= 3,
		STATUS_MISUSED 				= 4,
		STATUS_NETWORKERROR 		= 5,
		STATUS_MAX
	};
}

#endif //__LIBCOYOTE_STATUSCODES_H__
