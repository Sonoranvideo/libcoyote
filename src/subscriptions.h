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
#ifndef __LIBCOYOTE_SUBSCRIPTIONS_H__
#define __LIBCOYOTE_SUBSCRIPTIONS_H__

#define MSGPACK_DEFAULT_API_VERSION 2
#include "msgpack.hpp"
#include <mutex>

#include "include/common.h"
#include "include/datastructures.h"
#include "include/statuscodes.h"

namespace Subs
{
	struct StateEventCBSettings
	{
		Coyote::StateEventCallback CB;
		void *UserData;
	};
	
	class SubscriptionSession
	{
	private:
		std::mutex TimeCodesLock;
		std::mutex PresetsLock;
		std::mutex PresetStatesLock;
		std::mutex AssetsLock;
		std::mutex HWStateLock;
		std::unordered_map<int32_t, Coyote::TimeCode> TimeCodes;
		std::unordered_map<int32_t, Coyote::Preset> Presets;
		std::unordered_map<int32_t, Coyote::PresetState> PresetStates;
		std::unordered_map<std::string, Coyote::Asset> Assets;
		Coyote::KonaHardwareState HWState;
		Coyote::PBEventCallback UserPBEventCallback;
		void *UserPBEventData;
		Coyote::MiniviewCallback MiniviewCB;
		void *MiniviewCBData;
		
		StateEventCBSettings StateCallbacks[Coyote::COYOTE_STATE_MAX - 1];
		
	public:
		bool ProcessSubscriptionEvent(const std::unordered_map<std::string, msgpack::object> &Values);
		Coyote::TimeCode *GetTimeCode(const int32_t PK);
		std::unordered_map<int32_t, Coyote::Preset> *GetPresets(void);
		std::unordered_map<int32_t, Coyote::PresetState> *GetPresetStates(void);
		std::unordered_map<std::string, Coyote::Asset> *GetAssets(void);
		Coyote::KonaHardwareState *GetKonaHardwareState(void);
		void SetPlaybackEventCallback(const Coyote::PBEventCallback, void *const UserData = nullptr);
		void SetMiniviewCallback(const Coyote::MiniviewCallback, void *const UserData = nullptr);
		void SetStateEventCallback(const Coyote::StateEventType EType, const Coyote::StateEventCallback CB, void *const UserData);
		
		SubscriptionSession() : HWState(), UserPBEventCallback(), UserPBEventData(), MiniviewCB(), MiniviewCBData(), StateCallbacks() { }
	};
}
#endif //__LIBCOYOTE_SUBSCRIPTIONS_H__
