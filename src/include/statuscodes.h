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

#include <stdint.h>

#ifdef __cplusplus
namespace Coyote
{
	enum StatusCode
#else
	enum CoyoteStatusCode
#endif //__cplusplus
	{
		COYOTE_STATUS_INVALID 				= -1,
		COYOTE_STATUS_OK 					= 0,
		COYOTE_STATUS_FAILED 				= 1,
		COYOTE_STATUS_UNIMPLEMENTED 		= 2,
		COYOTE_STATUS_INTERNALERROR 		= 3,
		COYOTE_STATUS_MISUSED 				= 4,
		COYOTE_STATUS_NETWORKERROR 			= 5,
		COYOTE_STATUS_MAX
	};
#ifdef __cplusplus

	enum HardwareMode
#else
	enum Coyote_HardwareMode
#endif //__cplusplus
	{
		COYOTE_MODE_INVALID = -1,
		COYOTE_MODE_Q3G 	= 0,
		COYOTE_MODE_S12G	= 1,
		COYOTE_MODE_MAX
	};

#ifdef __cplusplus
	enum ResolutionMode
#else
	enum CoyoteResolutionMode
#endif
	{
		COYOTE_RES_INVALID = 0,
		COYOTE_RES_1080P = 1,
		COYOTE_RES_2160P = 2,
		COYOTE_RES_MAX
	};
	
#ifdef __cplusplus
	enum RefreshMode
#else
	enum CoyoteRefreshMode
#endif
	{
		COYOTE_REFRESH_INVALID 	= 0,
		COYOTE_REFRESH_23_98 	= 1,
		COYOTE_REFRESH_24 		= 2,
		COYOTE_REFRESH_25 		= 3,
		COYOTE_REFRESH_29_97 	= 4,
		COYOTE_REFRESH_30 		= 5,
		COYOTE_REFRESH_50 		= 6,
		COYOTE_REFRESH_59_94 	= 7,
		COYOTE_REFRESH_60 		= 8,
		COYOTE_REFRESH_MAX
	};
#ifdef __cplusplus
}

#endif //__cplusplus

#endif //__LIBCOYOTE_STATUSCODES_H__
