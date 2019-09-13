#include "subscriptions.h"
#include "msgpackproc.h"
#include <mutex>


bool Subs::SubscriptionSession::ProcessSubscriptionEvent(const std::map<std::string, msgpack::object> &Values)
{
	if (!Values.count("SubscriptionEvent") || !Values.count("Data")) return false; //Not a subscription event

	const std::string &EventName = Values.at("SubscriptionEvent").as<std::string>();
	
	if 		(EventName == "TimeCode")
	{
		std::unique_ptr<Coyote::TimeCode> TC { static_cast<Coyote::TimeCode*>(MsgpackProc::UnpackCoyoteObject(Values.at("Data"), typeid(Coyote::TimeCode))) };
		
		if (!TC) return false;
		
		std::lock_guard<std::mutex> G { this->TimeCodesLock };
		
		this->TimeCodes[TC->PresetKey] = *TC;

		if (TC->Selected)
		{
			this->TimeCodes[0] = *TC;
		}
		
		return true;
	}
	else if (EventName == "Presets")
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
		
		return true;
	}
	else if (EventName == "Assets")
	{
		LDEBUG_MSG("Decoding assets");
		std::vector<msgpack::object> AssetObjects;

		Values.at("Data").convert(AssetObjects);
	
		const std::lock_guard<std::mutex> G { this->AssetsLock };

		this->Assets.clear();
		
		for (auto Iter = AssetObjects.begin(); Iter != AssetObjects.end(); ++Iter)
		{
			std::unique_ptr<Coyote::Asset> Item { static_cast<Coyote::Asset*>(MsgpackProc::UnpackCoyoteObject(*Iter, typeid(Coyote::Asset))) };
			
			this->Assets.emplace(Item->FileName.GetStdString(), std::move(*Item));
		}
		
		return true;
	}
	else if (EventName == "HardwareState")
	{
		const std::lock_guard<std::mutex> G { this->HWStateLock };
		
		std::unique_ptr<Coyote::HardwareState> Ptr { static_cast<Coyote::HardwareState*>(MsgpackProc::UnpackCoyoteObject(Values.at("Data"), typeid(Coyote::HardwareState))) };
		
		this->HWState = std::move(*Ptr);
		
		return true;
	}

	return false;
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

Coyote::HardwareState *Subs::SubscriptionSession::GetHardwareState(void)
{
	const std::lock_guard<std::mutex> G { this->HWStateLock };
	
	return new Coyote::HardwareState { this->HWState }; //Call copy constructor
}
