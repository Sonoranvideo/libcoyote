#include "subscriptions.h"
#include "msgpackproc.h"
#include <mutex>


bool Subs::SubscriptionSession::ProcessSubscriptionEvent(const std::map<std::string, msgpack::object> &Values)
{
	if (!Values.count("SubscriptionEvent") || !Values.count("Data")) return false; //Not a subscription event

	const std::string &EventName = Values.at("SubscriptionEvent").as<std::string>();
	
	
	if (EventName == "TimeCode")
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
	
	return false;
}

Coyote::TimeCode *Subs::SubscriptionSession::GetTimeCode(const int32_t PK)
{
	std::lock_guard<std::mutex> G { this->TimeCodesLock };

	if (!this->TimeCodes.count(PK)) return nullptr;
	
	return new Coyote::TimeCode{ this->TimeCodes[PK] };
}
