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

#include "subscriptions.h"
#include "msgpackproc.h"
#include <mutex>

static const std::map<std::string, Coyote::StateEventType> CBMap
{
	{ "PresetsUpdate", Coyote::COYOTE_STATE_PRESETS },
	{ "PresetStatesUpdate", Coyote::COYOTE_STATE_PRESETSTATES },
	{ "TimeCode", Coyote::COYOTE_STATE_TIMECODE },
	{ "AssetSync", Coyote::COYOTE_STATE_ASSETS },
	{ "AssetPost", Coyote::COYOTE_STATE_ASSETS },
	{ "AssetDelete", Coyote::COYOTE_STATE_ASSETS },
	{ "KonaHardwareStateUpdate", Coyote::COYOTE_STATE_HWSTATE },
};
	
bool Subs::SubscriptionSession::ProcessSubscriptionEvent(const std::map<std::string, msgpack::object> &Values)
{
	if (!Values.count("SubscriptionEvent") || !Values.count("Data")) return false; //Not a subscription event

	const std::string &EventName = Values.at("SubscriptionEvent").as<std::string>();
	
	bool RetVal = false;
	
	if 		(EventName == "TimeCode")
	{
		std::unique_ptr<Coyote::TimeCode> TC { static_cast<Coyote::TimeCode*>(MsgpackProc::UnpackCoyoteObject(Values.at("Data"), typeid(Coyote::TimeCode))) };
		
		if (!TC)
		{
			RetVal = true;
			goto End;
		}
		
		std::lock_guard<std::mutex> G { this->TimeCodesLock };
		
		this->TimeCodes[TC->PK] = *TC;
		
		RetVal = true;
	}
	else if (EventName == "MiniviewPost")
	{
		if (this->MiniviewCB)
		{
			std::unordered_map<std::string, msgpack::object> Map;

			Values.at("Data").convert(Map);
			
			const int32_t PK = Map.at("PK").as<int32_t>();
			const uint32_t CanvasIndex = Map.at("CanvasIndex").as<uint32_t>();
			const uint32_t OutputNum = Map.at("OutputNum").as<uint32_t>();
			const int32_t Width = Map.at("Width").as<int32_t>();
			const int32_t Height = Map.at("Height").as<int32_t>();
			std::vector<uint8_t> Bytes;

			Map.at("Bytes").convert(Bytes);
			
			this->MiniviewCB(PK, CanvasIndex, OutputNum, Coyote::Size2D { Width, Height }, std::move(Bytes), this->MiniviewCBData);
		}

		RetVal = true;
	}
	else if (EventName == "PresetsUpdate")
	{
		std::vector<msgpack::object> PresetObjects;

		Values.at("Data").convert(PresetObjects);
	
		const std::lock_guard<std::mutex> G { this->PresetsLock };

		this->Presets.clear();
		
		for (auto Iter = PresetObjects.begin(); Iter != PresetObjects.end(); ++Iter)
		{
			std::unique_ptr<Coyote::Preset> Item { static_cast<Coyote::Preset*>(MsgpackProc::UnpackCoyoteObject(*Iter, typeid(Coyote::Preset))) };
			
			this->Presets.emplace(Item->PK, std::move(*Item));
		}
		
		RetVal = true;
	}
	else if (EventName == "PresetStatesUpdate")
	{
		std::vector<msgpack::object> PStateObjects;

		Values.at("Data").convert(PStateObjects);
	
		const std::lock_guard<std::mutex> G { this->PresetStatesLock };

		this->PresetStates.clear();
		
		for (auto Iter = PStateObjects.begin(); Iter != PStateObjects.end(); ++Iter)
		{
			std::unique_ptr<Coyote::PresetState> Item { static_cast<Coyote::PresetState*>(MsgpackProc::UnpackCoyoteObject(*Iter, typeid(Coyote::PresetState))) };
			
			this->PresetStates.emplace(Item->PK, std::move(*Item));
		}
		
		RetVal = true;
	}
	else if (EventName == "AssetSync")
	{
		LDEBUG_MSG("Decoding assets");
		std::vector<msgpack::object> AssetObjects;

		Values.at("Data").convert(AssetObjects);
	
		const std::lock_guard<std::mutex> G { this->AssetsLock };

		this->Assets.clear();
		
		for (auto Iter = AssetObjects.begin(); Iter != AssetObjects.end(); ++Iter)
		{
			std::unique_ptr<Coyote::Asset> Item { static_cast<Coyote::Asset*>(MsgpackProc::UnpackCoyoteObject(*Iter, typeid(Coyote::Asset))) };
			
			this->Assets.emplace(Item->FullPath, std::move(*Item));
		}
		
		RetVal = true;
	}
	else if (EventName == "AssetDelete")
	{
		LDEBUG_MSG("Decoding asset deletion request");
		std::string FullPath;

		Values.at("Data").convert(FullPath);
	
		const std::lock_guard<std::mutex> G { this->AssetsLock };
		
		if (this->Assets.count(FullPath))
		{
			LDEBUG_MSG("Found asset " << FullPath << " to delete, deleting.");
			this->Assets.erase(FullPath);
		}
		
		RetVal = true;
	}
	else if (EventName == "AssetPost")
	{
		LDEBUG_MSG("Decoding asset post request");
		
		std::unique_ptr<Coyote::Asset> Ptr { static_cast<Coyote::Asset*>(MsgpackProc::UnpackCoyoteObject(Values.at("Data"), typeid(Coyote::Asset))) };
	
		const std::lock_guard<std::mutex> G { this->AssetsLock };
		
		this->Assets[Ptr->FullPath] = std::move(*Ptr);
		
		LDEBUG_MSG("Found asset " << Ptr->FullPath << " to add/update.");
		
		RetVal = true;
	}
	else if (EventName == "KonaHardwareStateUpdate")
	{
		const std::lock_guard<std::mutex> G { this->HWStateLock };
		
		std::unique_ptr<Coyote::KonaHardwareState> Ptr { static_cast<Coyote::KonaHardwareState*>(MsgpackProc::UnpackCoyoteObject(Values.at("Data"), typeid(Coyote::KonaHardwareState))) };
		
		this->HWState = std::move(*Ptr);
		
		RetVal = true;
	}
	else if (EventName == "PlaybackEvent")
	{
		std::map<std::string, msgpack::object> DataObjs;
		
		Values.at("Data").convert(DataObjs);
		
		const Coyote::PlaybackEventType EType = static_cast<Coyote::PlaybackEventType>(DataObjs.at("EType").as<int>());
		const int32_t PK = DataObjs.at("PK").as<int32_t>();
		const int32_t NewTime = DataObjs.count("NewTime") ? DataObjs.at("NewTime").as<int32_t>() : 0;
		
		if (this->UserPBEventCallback)
		{
			this->UserPBEventCallback(EType, PK, NewTime, this->UserPBEventData);
		}
		
		RetVal = true;
	}
End:

	if (CBMap.count(EventName))
	{
		const auto &Struct { this->StateCallbacks[CBMap.at(EventName) - 1] };
		
		if (Struct.CB != nullptr)
		{
			LDEBUG_MSG("Invoking callback for EType " << EventName);
			Struct.CB(CBMap.at(EventName), Struct.UserData);
		}
	}
	else
	{
		LDEBUG_MSG("Invalid EventName " << EventName);
	}
	
	return RetVal;
}

Coyote::TimeCode *Subs::SubscriptionSession::GetTimeCode(const int32_t PK)
{
	std::lock_guard<std::mutex> G { this->TimeCodesLock };

	if (!this->TimeCodes.count(PK)) return nullptr;
	
	return new Coyote::TimeCode{ this->TimeCodes[PK] };
}

std::map<std::string, Coyote::Asset> *Subs::SubscriptionSession::GetAssets(void)
{
	const std::lock_guard<std::mutex> G { this->AssetsLock };
	
	return new decltype(this->Assets) { this->Assets }; //Copy, not move.
}

std::map<int32_t, Coyote::Preset> *Subs::SubscriptionSession::GetPresets(void)
{
	const std::lock_guard<std::mutex> G { this->PresetsLock };
	
	return new decltype(this->Presets) { this->Presets }; //Copy, not move.
}

std::map<int32_t, Coyote::PresetState> *Subs::SubscriptionSession::GetPresetStates(void)
{
	const std::lock_guard<std::mutex> G { this->PresetStatesLock };
	
	return new decltype(this->PresetStates) { this->PresetStates }; //Copy, not move.
}

Coyote::KonaHardwareState *Subs::SubscriptionSession::GetKonaHardwareState(void)
{
	const std::lock_guard<std::mutex> G { this->HWStateLock };
	
	return new Coyote::KonaHardwareState { this->HWState }; //Call copy constructor
}


void Subs::SubscriptionSession::SetStateEventCallback(const Coyote::StateEventType EType, const Coyote::StateEventCallback CB, void *const UserData)
{
	if (EType > Coyote::COYOTE_STATE_MAX || !EType)
	{
		LDEBUG_MSG("ERROR, state event type value " << (int)EType << " is greater than max value " << Coyote::COYOTE_STATE_MAX - 1);
		return;
	}
	
	LDEBUG_MSG("Setting state event callback for EType " << EType);
	
	this->StateCallbacks[EType - 1] = { CB, UserData };
}

void Subs::SubscriptionSession::SetPlaybackEventCallback(const Coyote::PBEventCallback CB, void *const UserData)
{
	this->UserPBEventCallback = CB;
	this->UserPBEventData = UserData;
}

void Subs::SubscriptionSession::SetMiniviewCallback(const Coyote::MiniviewCallback CB, void *const UserData)
{
	this->MiniviewCB = CB;
	this->MiniviewCBData = UserData;
}

