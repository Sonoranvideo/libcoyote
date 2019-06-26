#include "include/internal/common.h"
#include "include/internal/msgpackproc.h"
#include "include/libcoyote.h"

#ifdef MSGPACK_DEFAULT_API_VERSION
#undef MSGPACK_DEFAULT_API_VERSION
#endif

#define MSGPACK_DEFAULT_API_VERSION 2

#include <msgpack.hpp>

#include <typeinfo>

//Prototypes
static bool HasValidIncomingHeaders(const std::map<std::string, msgpack::object> &Values);

//Definitions
void MsgpackProc::InitOutgoingMsg(msgpack::packer<msgpack::sbuffer> &Pack, const std::string &CommandName, const uint64_t MsgID, const msgpack::object *Values)
{
	std::map<std::string, msgpack::object> TotalValues
	{
		{ "CommandName", msgpack::object{ CommandName.c_str() } },
		{ "CoyoteAPIVersion", msgpack::object{ COYOTE_API_VERSION } },
		{ "MsgID", msgpack::object{ MsgID } },
	};

	if (Values != nullptr) TotalValues["Data"] = *Values;
	
	Pack.pack(TotalValues);
}

static bool HasValidIncomingHeaders(const std::map<std::string, msgpack::object> &Values)
{
	static const char *const Required[] = { "CommandName", "CoyoteAPIVersion", "MsgID", "StatusInt", "StatusText" };
	static const size_t NumFields = sizeof Required / sizeof *Required;
	
	for (size_t Inc = 0u; Inc < NumFields; ++Inc)
	{
		if (!Values.count(Required[Inc])) return false;
	}
	
	return true;
}

std::map<std::string, msgpack::object> MsgpackProc::InitIncomingMsg(const void *Data, const size_t DataLength, uint64_t *MsgIDOut)
{
	//Unpack binary data.
	msgpack::unpacked Result;
	msgpack::unpack(Result, static_cast<const char*>(Data), DataLength);
	
	//Get object from handle.
	msgpack::object Object(std::move(Result.get()));
	
	//Convert into a map of smaller msgpack objects
	std::map<std::string, msgpack::object> Values;
	Object.convert(Values);
	
	//malformed message?
	assert(HasValidIncomingHeaders(Values));
	
	//Easy way to get the event ID
	if (MsgIDOut) *MsgIDOut = Values["MsgID"].as<uint64_t>();
	
	return Values;
}

msgpack::object MsgpackProc::PackCoyoteObject(const Coyote::BaseObject *Object, msgpack::packer<msgpack::sbuffer> *Pack)
{ //I haven't used typeid/RTTI since 2014. I figured 'why not', it's here anyways.
	
	const std::type_info &OurType { typeid(*Object) };
	
	std::map<std::string, msgpack::object> *Values = nullptr;
	
	if 		(OurType == typeid(Coyote::Asset))
	{
		const Coyote::Asset *AssetObj = static_cast<const Coyote::Asset*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "FileName", msgpack::object{ AssetObj->FileName } },
			{ "NewFileName", msgpack::object{ AssetObj->NewFileName } },
			{ "CopyPercentage", msgpack::object{ AssetObj->CopyPercentage } },
			{ "IsReady", msgpack::object{ AssetObj->IsReady } }
		};
	}
	else if (OurType == typeid(Coyote::HardwareState))
	{
		const Coyote::HardwareState *HWStateObj = static_cast<const Coyote::HardwareState*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "SupportsS12G", msgpack::object{ HWStateObj->SupportsS12G } },
			{ "Resolution", msgpack::object{ HWStateObj->Resolution } },
			{ "RefreshRate", msgpack::object{ HWStateObj->RefreshRate } },
			{ "CurrentMode", msgpack::object{ static_cast<int>(HWStateObj->CurrentMode) } },
		};
	}
	else if (OurType == typeid(Coyote::TimeCode))
	{
		const Coyote::TimeCode *TCObj = static_cast<const Coyote::TimeCode*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "TRT", msgpack::object{ TCObj->TRT } },
			{ "Time", msgpack::object{ TCObj->Time } },
			{ "ScrubBar", msgpack::object{ TCObj->ScrubBar } },
		};
	}
	else if (OurType == typeid(Coyote::MediaState))
	{
		const Coyote::MediaState *MediaStateObj = static_cast<const Coyote::MediaState*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "NumPresets", msgpack::object{ MediaStateObj->NumPresets } },
			{ "Selected", msgpack::object{ MediaStateObj->Selected } },
			{ "PlayingPresets", msgpack::object{ MediaStateObj->PlayingPresets } },
			{ "PausedPresets", msgpack::object{ MediaStateObj->PausedPresets } },
			{ "TimeCode", PackCoyoteObject(&MediaStateObj->Time) },
		};			
	}
	else if (OurType == typeid(Coyote::NetworkInfo))
	{
		const Coyote::NetworkInfo *NetInfo = static_cast<const Coyote::NetworkInfo*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "IP", msgpack::object{ NetInfo->IP.GetCString() } },
			{ "Subnet", msgpack::object{ NetInfo->Subnet.GetCString() } },
			{ "AdapterID", msgpack::object{ NetInfo->AdapterID } },
		};
	}
	else if (OurType == typeid(Coyote::Output))
	{
		const Coyote::Output *OutputObj = static_cast<const Coyote::Output*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "Filename", msgpack::object{ OutputObj->Filename.GetCString() } },
			{ "Hue", msgpack::object{ OutputObj->Hue } },
			{ "Saturation", msgpack::object{ OutputObj->Saturation } },
			{ "Contrast", msgpack::object{ OutputObj->Contrast } },
			{ "Brightness", msgpack::object{ OutputObj->Brightness } },
			{ "MediaId", msgpack::object{ OutputObj->MediaId } },
			{ "FadeOut", msgpack::object{ OutputObj->FadeOut } },
			{ "Delay", msgpack::object{ OutputObj->Delay } },
			{ "Active", msgpack::object{ OutputObj->Active } },
			{ "Audio", msgpack::object{ OutputObj->Audio } },
		};
	}
	else if (OurType == typeid(Coyote::Preset))
	{
		const Coyote::Preset *PresetObj = static_cast<const Coyote::Preset*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "Output1", msgpack::object{ PackCoyoteObject(&PresetObj->Output1) } },
			{ "Output2", msgpack::object{ PackCoyoteObject(&PresetObj->Output2) } },
			{ "Output3", msgpack::object{ PackCoyoteObject(&PresetObj->Output3) } },
			{ "Output4", msgpack::object{ PackCoyoteObject(&PresetObj->Output4) } },
			{ "Name", msgpack::object{ PresetObj->Name.GetCString() } },
			{ "Layout", msgpack::object{ PresetObj->Layout.GetCString() } },
			{ "Notes", msgpack::object{ PresetObj->Notes.GetCString() } },
			{ "Color", msgpack::object{ PresetObj->Color.GetCString() } },
			{ "PK", msgpack::object{ PresetObj->PK } },
			{ "Index", msgpack::object{ PresetObj->Index } },
			{ "Loop", msgpack::object{ PresetObj->Loop } },
			{ "Link", msgpack::object{ PresetObj->Link } },
			{ "DisplayLink", msgpack::object{ PresetObj->DisplayLink } },
			{ "Fade", msgpack::object{ PresetObj->Fade } },
			{ "LeftVolume", msgpack::object{ PresetObj->LeftVolume } },
			{ "RightVolume", msgpack::object{ PresetObj->RightVolume } },
			{ "IsPlaying", msgpack::object{ PresetObj->IsPlaying } },
			{ "IsPaused", msgpack::object{ PresetObj->IsPaused } },
			{ "Selected", msgpack::object{ PresetObj->Selected } },
		};
	}

	assert(Values != nullptr);
	
	if (Pack)
	{
		Pack->pack(*Values);
	}
	
	return msgpack::object{ *Values };
	
}

Coyote::BaseObject *MsgpackProc::UnpackCoyoteObject(const msgpack::object &Object, const std::type_info &Expected)
{
	Coyote::BaseObject *Result = nullptr;
	
	std::map<std::string, msgpack::object> Fields;
	
	Object.convert(Fields);
	
	if 		(Expected == typeid(Coyote::Output))
	{
		Result = new Coyote::Output{};
		Coyote::Output *OutputObj = static_cast<Coyote::Output*>(Result);
		
		OutputObj->Filename = Fields["Filename"].as<std::string>();
		OutputObj->Hue = Fields["Hue"].as<int32_t>();
		OutputObj->Saturation = Fields["Saturation"].as<int32_t>();
		OutputObj->Contrast = Fields["Brightness"].as<int32_t>();
		OutputObj->MediaId = Fields["MediaId"].as<int32_t>();
		OutputObj->FadeOut = Fields["FadeOut"].as<double>();
		OutputObj->Delay = Fields["Delay"].as<double>();
		OutputObj->Active = Fields["Active"].as<bool>();
		OutputObj->Audio = Fields["Audio"].as<bool>();
	}
	else if (Expected == typeid(Coyote::Asset))
	{
		Result = new Coyote::Asset{};
		Coyote::Asset *AssetObj = static_cast<Coyote::Asset*>(Result);
		
		AssetObj->FileName = Fields["FileName"].as<std::string>();
		AssetObj->NewFileName = Fields["NewFileName"].as<std::string>();
		AssetObj->CopyPercentage = Fields["CopyPercentage"].as<uint32_t>(); //Because idk if msgpack treats uint8_t like a character or a number
		AssetObj->IsReady = Fields["CopyPercentage"].as<bool>();
	}
	else if (Expected == typeid(Coyote::TimeCode))
	{
		Result = new Coyote::TimeCode{};
		Coyote::TimeCode *TCObj = static_cast<Coyote::TimeCode*>(Result);
		
		TCObj->TRT = Fields["TRT"].as<uint32_t>();
		TCObj->Time = Fields["Time"].as<uint32_t>();
		TCObj->ScrubBar = Fields["ScrubBar"].as<double>();
	}
	else if (Expected == typeid(Coyote::Preset))
	{
		Result = new Coyote::Preset{};
		Coyote::Preset *PresetObj = static_cast<Coyote::Preset*>(Result);
		
		PresetObj->Name = Fields["Name"].as<std::string>();
		PresetObj->Layout = Fields["Layout"].as<std::string>();
		PresetObj->Notes = Fields["Notes"].as<std::string>();
		PresetObj->Color = Fields["Color"].as<std::string>();
		PresetObj->PK = Fields["PK"].as<int>();
		PresetObj->Index = Fields["Index"].as<int>();
		PresetObj->Loop = Fields["Loop"].as<int>();
		PresetObj->Link = Fields["Link"].as<int>();
		PresetObj->DisplayLink = Fields["DisplayLink"].as<int>();
		PresetObj->Fade = Fields["Fade"].as<int>();
		PresetObj->LeftVolume = Fields["LeftVolume"].as<int>();
		PresetObj->RightVolume = Fields["RightVolume"].as<int>();
		PresetObj->IsPlaying = Fields["IsPlaying"].as<bool>();
		PresetObj->IsPaused = Fields["IsPaused"].as<bool>();
		PresetObj->Selected = Fields["Selected"].as<bool>();
	}
	else if (Expected == typeid(Coyote::HardwareState))
	{
		Result = new Coyote::HardwareState{};
		Coyote::HardwareState *HWObj = static_cast<Coyote::HardwareState*>(Result);
		
		HWObj->SupportsS12G = Fields["SupportsS12G"].as<bool>();
		HWObj->Resolution = Fields["Resolution"].as<std::string>();
		HWObj->RefreshRate = Fields["RefreshRate"].as<std::string>();
		HWObj->CurrentMode = static_cast<Coyote::HardwareMode>(Fields["CurrentMode"].as<int>());
	}
	else if (Expected == typeid(Coyote::MediaState))
	{
		Result = new Coyote::MediaState{};
		Coyote::MediaState *MSObj = static_cast<Coyote::MediaState*>(Result);
		
		std::vector<uint32_t> PlayingPresets;
		std::vector<uint32_t> PausedPresets;
		
		Fields["PlayingPresets"].convert(PlayingPresets);
		Fields["PausedPresets"].convert(PausedPresets);
		
		assert(MSObj->PlayingPresets.size() >= PlayingPresets.size());
		assert(MSObj->PausedPresets.size() >= PausedPresets.size());
		
		//Not sure msgpack.hpp can handle std::array, not gonna find out
		memcpy(MSObj->PlayingPresets.data(), PlayingPresets.data(), PlayingPresets.size());
		memcpy(MSObj->PausedPresets.data(), PausedPresets.data(), PausedPresets.size());
		
		MSObj->NumPresets = Fields["NumPresets"].as<int>();
		MSObj->Selected = Fields["Selected"].as<int>();
		
		std::unique_ptr<Coyote::BaseObject> TC { UnpackCoyoteObject(Fields["TimeCode"], typeid(Coyote::TimeCode)) };
		
		MSObj->Time = *static_cast<Coyote::TimeCode*>(TC.get());
	}
	else if (Expected == typeid(Coyote::NetworkInfo))
	{
		Result = new Coyote::NetworkInfo{};
		Coyote::NetworkInfo *InfoObj = static_cast<Coyote::NetworkInfo*>(Result);
		
		InfoObj->IP = Fields["IP"].as<std::string>();
		InfoObj->Subnet = Fields["Subnet"].as<std::string>();
		InfoObj->AdapterID = Fields["AdapterID"].as<uint32_t>();
	}
	
	assert(Result != nullptr);
	
	return Result;
}
