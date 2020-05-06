/*
   Copyright 2020 Sonoran Video Systems

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
	enum CoyoteHardwareMode
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
#endif //__cplusplus
	{
		COYOTE_RES_INVALID = 0,
		COYOTE_RES_1080P = 1,
		COYOTE_RES_2160P = 2,
		COYOTE_RES_1080I = 3,
		COYOTE_RES_MAX
	};
	
#ifdef __cplusplus
	enum UnitRole
#else
	enum CoyoteUnitRole
#endif //__cplusplus
	{
		COYOTE_ROLE_INVALID = 0,
		COYOTE_ROLE_SINGLE = 1,
		COYOTE_ROLE_PRIMARY = 2,
		COYOTE_ROLE_MIRROR = 3,
		COYOTE_ROLE_MAX
	};
#ifdef __cplusplus
	enum RefreshMode
#else
	enum CoyoteRefreshMode
#endif //__cplusplus
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
	enum PresetLayout
#else
	enum CoyotePresetLayout
#endif //__cplusplus
	{ //Out of order because support is being added incrementally for different layouts
		COYOTE_PSLAYOUT_INVALID = 0,
		COYOTE_PSLAYOUT_A		= 1,
		COYOTE_PSLAYOUT_B		= 2,
		COYOTE_PSLAYOUT_C1		= 3,
		COYOTE_PSLAYOUT_C2		= 4,
		COYOTE_PSLAYOUT_C3		= 5,
		COYOTE_PSLAYOUT_F		= 6,
		COYOTE_PSLAYOUT_E		= 7,
		COYOTE_PSLAYOUT_D1		= 8,
		COYOTE_PSLAYOUT_D2		= 9,
		COYOTE_PSLAYOUT_MAX
	};

#ifdef __cplusplus
	enum SDIOutput
#else
	enum CoyoteSDIOutput
#endif //__cplusplus
	{
		COYOTE_SDI_INVALID = 0,
		COYOTE_SDI_1 = 1 << 0,
		COYOTE_SDI_2 = 1 << 1,
		COYOTE_SDI_3 = 1 << 2,
		COYOTE_SDI_4 = 1 << 3,
		COYOTE_SDI_MAXVALUE = COYOTE_SDI_4
	};
	
#ifdef __cplusplus
	enum HDRMode
#else
	enum CoyoteHDRMode
#endif //__cplusplus
	{
		COYOTE_HDR_DISABLED = 0,
		COYOTE_HDR_BT2020,
		COYOTE_HDR_DCI_P3,
		COYOTE_HDR_DOLBY,
		COYOTE_HDR_MAX
	};
	
#ifdef __cplusplus
	enum EOTFMode
#else
	enum CoyoteEOTFMode
#endif //__cplusplus
	{
		COYOTE_EOTF_NORMAL = 0,
		COYOTE_EOTF_HLG = 1,
		COYOTE_EOTF_PQ = 2,
		COYOTE_EOTF_UNSPECIFIED = 3,
		COYOTE_EOTF_MAX
	};
		
#ifdef __cplusplus
	enum Player
#else
	enum CoyotePlayer
#endif //__cplusplus
	{
		COYOTE_PLAYER_INVALID = 0,
		COYOTE_PLAYER_1 = 1 << 0,
		COYOTE_PLAYER_2 = 1 << 1,
		COYOTE_PLAYER_3 = 1 << 2,
		COYOTE_PLAYER_4 = 1 << 3,
		COYOTE_PLAYER_MAXVALUE = COYOTE_PLAYER_4
	};
	
#ifdef __cplusplus
	enum PlaybackEventType
#else
	enum CoyotePlaybackEventType
#endif //__cplusplus
	{
		COYOTE_PBEVENT_END = 0,
		COYOTE_PBEVENT_TAKE = 1,
		COYOTE_PBEVENT_PAUSE = 2,
		COYOTE_PBEVENT_UNPAUSE = 3,
		COYOTE_PBEVENT_SEEK = 4
	};
	
#ifdef __cplusplus
	enum AssetState
#else
	enum CoyoteAssetState
#endif //__cplusplus
	{
		COYOTE_ASSETSTATE_INVALID	= 0,
		COYOTE_ASSETSTATE_READY		= 1,
		COYOTE_ASSETSTATE_INGESTING	= 2,
		COYOTE_ASSETSTATE_ERROR		= 3,
		COYOTE_ASSETSTATE_PROCESSING= 4,
		COYOTE_ASSETSTATE_MAX
	};
	
#ifdef __cplusplus
	enum StateEventType
#else
	enum CoyoteStateEventType
#endif //__cplusplus
	{
		COYOTE_STATE_INVALID 	= 0,
		COYOTE_STATE_PRESETS 	= 1,
		COYOTE_STATE_ASSETS 	= 2,
		COYOTE_STATE_HWSTATE 	= 3,
		COYOTE_STATE_TIMECODE 	= 4,
		COYOTE_STATE_MAX
	};

#ifndef __cplusplus
	typedef enum CoyoteResolutionMode CoyoteResolutionMode;
	typedef enum CoyoteRefreshMode CoyoteRefreshMode;
	typedef enum CoyoteHardwareMode CoyoteHardwareMode;
	typedef enum CoyoteStatusCode CoyoteStatusCode;
	typedef enum CoyotePresetLayout CoyotePresetLayout;
	typedef enum CoyoteSDIOutput CoyoteSDIOutput;
	typedef enum CoyoteEOTFMode CoyoteEOTFMode;
	typedef enum CoyoteHDRMode CoyoteHDRMode;
#else
}
#endif // !__cplusplus


#endif //__LIBCOYOTE_STATUSCODES_H__
