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

#ifndef __LIBCOYOTE_STATUSCODES_H__
#define __LIBCOYOTE_STATUSCODES_H__

#include "common.h"

namespace Coyote
{
	enum StatusCode
	{
		COYOTE_STATUS_INVALID 				= -1,
		COYOTE_STATUS_OK 					= 0,
		COYOTE_STATUS_FAILED 				= 1,
		COYOTE_STATUS_UNIMPLEMENTED 		= 2,
		COYOTE_STATUS_INTERNALERROR 		= 3,
		COYOTE_STATUS_MISUSED 				= 4,
		COYOTE_STATUS_NETWORKERROR 			= 5,
		COYOTE_STATUS_UNSUPPORTED			= 6,
		COYOTE_STATUS_MAX
	};

	enum ResolutionMode
	{
		COYOTE_RES_INVALID = 0,
		COYOTE_RES_720P	 = 1,
		COYOTE_RES_1080P = 2,
		COYOTE_RES_2160P = 3,
		COYOTE_RES_1080I = 4,
		COYOTE_RES_MAX
	};
	
	enum UnitRole
	{
		COYOTE_ROLE_INVALID = 0,
		COYOTE_ROLE_SINGLE = 1,
		COYOTE_ROLE_PRIMARY = 2,
		COYOTE_ROLE_MIRROR = 3,
		COYOTE_ROLE_MAX
	};
	
	
	enum UnitType
	{
		COYOTE_UTYPE_INVALID	= 0,
		COYOTE_UTYPE_SERVER_K4	= 1,
		COYOTE_UTYPE_SERVER_K5	= 2,
		COYOTE_UTYPE_FLEX		= 3,
		COYOTE_UTYPE_MAX
	};
	
	enum EasyCanvasAlignment
	{
		COYOTE_ECALIGN_INVALID = 0,
		COYOTE_ECALIGN_TOPLEFT = 1,
		COYOTE_ECALIGN_TOPRIGHT = 2,
		COYOTE_ECALIGN_BOTTOMLEFT = 3,
		COYOTE_ECALIGN_BOTTOMRIGHT = 4,
		COYOTE_ECALIGN_CENTERED = 5,
		COYOTE_ECALIGN_SCALED = 6,
		COYOTE_ECALIGN_STRETCHED = 7,
		COYOTE_ECALIGN_MAX
	};
	
	enum SinkType
	{
		COYOTE_SINK_INVALID = 0,
		COYOTE_SINK_DEMO = 1,
		COYOTE_SINK_KONA = 1 << 1,
		COYOTE_SINK_NDI = 1 << 2,
		COYOTE_SINK_MAXVALUE = COYOTE_SINK_NDI
	};
	
	enum RefreshMode
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
	
	enum SDIOutput
	{
		COYOTE_SDI_INVALID = 0,
		COYOTE_SDI_1 = 1 << 0,
		COYOTE_SDI_2 = 1 << 1,
		COYOTE_SDI_3 = 1 << 2,
		COYOTE_SDI_4 = 1 << 3,
		COYOTE_SDI_MAXVALUE = COYOTE_SDI_4
	};
	
	enum HDRMode
	{
		COYOTE_HDR_DISABLED = 0,
		COYOTE_HDR_BT2020,
		COYOTE_HDR_DCI_P3,
		COYOTE_HDR_DOLBY,
		COYOTE_HDR_MAX
	};
	
	enum EOTFMode
	{
		COYOTE_EOTF_NORMAL = 0,
		COYOTE_EOTF_HLG = 1,
		COYOTE_EOTF_PQ = 2,
		COYOTE_EOTF_UNSPECIFIED = 3,
		COYOTE_EOTF_MAX
	};
		
	enum Player
	{	/*What's important to understand about "players" is, they're totally an illusion, not even an abstraction.
		* The internal Coyote code has no such notion of "players", at least as this represents them.
		* They're only here to make the UI seem easier and more logical to the users.
		* In reality, "player 1" is the first renderer created, and "player 2" is the second created, regardless of what outputs they do or don't span.
		* There is no disabled "player 2" and "player 4" if you span two 3840x1080s, etc.
		* All the player-related utility functions do is compute a value that you can use to put in a GUI, if you decide you want to mimic our look.
		*/
		COYOTE_PLAYER_INVALID = 0,
		COYOTE_PLAYER_1 = 1 << 0,
		COYOTE_PLAYER_2 = 1 << 1,
		COYOTE_PLAYER_3 = 1 << 2,
		COYOTE_PLAYER_4 = 1 << 3,
		COYOTE_PLAYER_MAXVALUE = COYOTE_PLAYER_4
	};
	
	enum PlaybackEventType
	{
		COYOTE_PBEVENT_END = 0,
		COYOTE_PBEVENT_TAKE = 1,
		COYOTE_PBEVENT_PAUSE = 2,
		COYOTE_PBEVENT_UNPAUSE = 3,
		COYOTE_PBEVENT_SEEK = 4
	};
	
	enum AssetState
	{
		COYOTE_ASSETSTATE_INVALID	= 0,
		COYOTE_ASSETSTATE_READY		= 1,
		COYOTE_ASSETSTATE_INGESTING	= 2,
		COYOTE_ASSETSTATE_ERROR		= 3,
		COYOTE_ASSETSTATE_PROCESSING= 4,
		COYOTE_ASSETSTATE_MAX
	};
	
	enum StateEventType
	{
		COYOTE_STATE_INVALID		= 0,
		COYOTE_STATE_PRESETS		= 1,
		COYOTE_STATE_PRESETSTATES 	= 2,
		COYOTE_STATE_ASSETS			= 3,
		COYOTE_STATE_HWSTATE		= 4,
		COYOTE_STATE_TIMECODE		= 5,
		COYOTE_STATE_MAX
	};
	
	enum KonaAudioConfig
	{	
		COYOTE_KAC_DISABLED = 0,
		COYOTE_KAC_SDI1 = 1 << 0, //Dump all audio channels to a particular SDI.
		COYOTE_KAC_SDI2 = 1 << 1,
		COYOTE_KAC_SDI3 = 1 << 2,
		COYOTE_KAC_SDI4 = 1 << 3,
		COYOTE_KAC_MAX_SDI = COYOTE_KAC_SDI4,
		COYOTE_KAC_DUALMODE_FLAG = 1 << 4, //If this flag is set, the channels are split between two other SDI specified as flags.
		COYOTE_KAC_MAX,
	};

}

MSGPACK_ADD_ENUM(Coyote::StatusCode)
MSGPACK_ADD_ENUM(Coyote::ResolutionMode)
MSGPACK_ADD_ENUM(Coyote::UnitRole)
MSGPACK_ADD_ENUM(Coyote::UnitType)
MSGPACK_ADD_ENUM(Coyote::RefreshMode)
MSGPACK_ADD_ENUM(Coyote::SDIOutput)
MSGPACK_ADD_ENUM(Coyote::HDRMode)
MSGPACK_ADD_ENUM(Coyote::EOTFMode)
MSGPACK_ADD_ENUM(Coyote::Player)
MSGPACK_ADD_ENUM(Coyote::PlaybackEventType)
MSGPACK_ADD_ENUM(Coyote::AssetState)
MSGPACK_ADD_ENUM(Coyote::StateEventType)
MSGPACK_ADD_ENUM(Coyote::SinkType)
MSGPACK_ADD_ENUM(Coyote::KonaAudioConfig)


#endif //__LIBCOYOTE_STATUSCODES_H__
