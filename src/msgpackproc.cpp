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
#include "common.h"
#include "msgpackproc.h"
#include "include/libcoyote.h"

#ifdef MSGPACK_DEFAULT_API_VERSION
#undef MSGPACK_DEFAULT_API_VERSION
#endif

#define MSGPACK_DEFAULT_API_VERSION 2

#include <msgpack.hpp>

#include <typeinfo>

//Prototypes
static bool HasValidIncomingHeaders(const std::map<std::string, msgpack::object> &Values);
static bool IsSubscriptionEvent(const std::map<std::string, msgpack::object> &Values);

thread_local msgpack::zone &MsgpackProc::Zone = *new msgpack::zone; //I don't want this on the stack because I don't trust it.

extern const std::map<Coyote::ResolutionMode, std::string> ResolutionMap;
extern const std::map<Coyote::RefreshMode, std::string> RefreshMap;

Coyote::RefreshMode ReverseRefreshMap(const std::string &Lookup);
Coyote::ResolutionMode ReverseResolutionMap(const std::string &Lookup);

//Definitions
void MsgpackProc::InitOutgoingMsg(msgpack::packer<msgpack::sbuffer> &Pack, const std::string &CommandName, const uint64_t MsgID, const msgpack::object *Values)
{
	std::map<std::string, msgpack::object> TotalValues
	{
		{ "CommandName", msgpack::object{ CommandName.c_str(), Zone } },
		{ "CoyoteAPIVersion", msgpack::object{ COYOTE_API_VERSION, Zone } },
	};
	
	if (MsgID) TotalValues.emplace("MsgID", msgpack::object{ MsgID, Zone });
	
	if (Values != nullptr) TotalValues["Data"] = *Values;

	Pack.pack(TotalValues);
}

static bool HasValidIncomingHeaders(const std::map<std::string, msgpack::object> &Values)
{
	static const char *const Required[] = { "CommandName", "CoyoteAPIVersion", "StatusInt", "StatusText" };
	static const size_t NumFields = sizeof Required / sizeof *Required;
	
	for (size_t Inc = 0u; Inc < NumFields; ++Inc)
	{
		if (!Values.count(Required[Inc])) return false;
	}
	
	return true;
}

static bool IsSubscriptionEvent(const std::map<std::string, msgpack::object> &Values)
{
	return Values.count("SubscriptionEvent");
}

std::map<std::string, msgpack::object> MsgpackProc::InitIncomingMsg(const void *Data, const size_t DataLength, uint64_t *MsgIDOut)
{
	//Unpack binary data.
	msgpack::unpacked Result;

	msgpack::object Object { msgpack::unpack(Zone, (const char*)Data, DataLength) };
	
	//Convert into a map of smaller msgpack objects
	std::map<std::string, msgpack::object> Values;
	Object.convert(Values);
	
	//Subscription events just get converted and done.
	if (IsSubscriptionEvent(Values)) return Values;
	
	//malformed message?
	assert(HasValidIncomingHeaders(Values));
	
	//Easy way to get the event ID
	if (MsgIDOut && Values.count("MsgID")) *MsgIDOut = Values["MsgID"].as<uint64_t>();
	
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
			{ "FileName", msgpack::object{ AssetObj->FileName.GetCString() } },
			{ "NewFileName", msgpack::object{ AssetObj->NewFileName.GetCString() } },
			{ "CopyPercentage", msgpack::object{ (int)AssetObj->CopyPercentage } },
			{ "IsReady", msgpack::object{ AssetObj->IsReady } }
		};
	}
	else if (OurType == typeid(Coyote::HardwareState))
	{
		const Coyote::HardwareState *HWStateObj = static_cast<const Coyote::HardwareState*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "SupportsS12G", msgpack::object{ HWStateObj->SupportsS12G } },
			{ "Resolution", msgpack::object{ ResolutionMap.at(HWStateObj->Resolution).c_str() } },
			{ "RefreshRate", msgpack::object{ RefreshMap.at(HWStateObj->RefreshRate).c_str() } },
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
			{ "Selected", msgpack::object{ TCObj->Selected } },
			{ "PresetKey", msgpack::object{ TCObj->PresetKey } },
		};
	}
	else if (OurType == typeid(Coyote::PresetMark))
	{
		const Coyote::PresetMark *MarkObj = static_cast<const Coyote::PresetMark*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "MarkNumber", msgpack::object { MarkObj->MarkNumber.GetCString() } },
			{ "MarkName", msgpack::object { MarkObj->MarkName.GetCString() } },
			{ "MarkDisplayTime", msgpack::object { MarkObj->MarkDisplayTime.GetCString() } },
			{ "MarkTime", msgpack::object { MarkObj->MarkTime } },
			
		};
	}
	else if (OurType == typeid(Coyote::MediaState))
	{
		const Coyote::MediaState *MediaStateObj = static_cast<const Coyote::MediaState*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "NumPresets", msgpack::object{ MediaStateObj->NumPresets } },
			{ "SelectedPreset", msgpack::object{ MediaStateObj->SelectedPreset } },
			{ "PlayingPresets", msgpack::object{ STLArrayToMsgpackArray(MediaStateObj->PlayingPresets) } },
			{ "PausedPresets", msgpack::object{ STLArrayToMsgpackArray(MediaStateObj->PausedPresets) } },
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
	else if (OurType == typeid(Coyote::Mirror))
	{
		const Coyote::Mirror *MirrorInfo = static_cast<const Coyote::Mirror*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "IP", msgpack::object{ MirrorInfo->IP.GetCString() } },
			{ "Busy", msgpack::object{ MirrorInfo->Busy } },
			{ "SupportsS12G", msgpack::object{ MirrorInfo->SupportsS12G } },
			{ "UnitID", msgpack::object{ MirrorInfo->UnitID.GetCString() } },
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
			{ "AudioChannel1", msgpack::object{ OutputObj->AudioChannel1 } },
			{ "AudioChannel2", msgpack::object{ OutputObj->AudioChannel2 } },
			{ "AudioChannel3", msgpack::object{ OutputObj->AudioChannel3 } },
			{ "AudioChannel4", msgpack::object{ OutputObj->AudioChannel4 } },
			
		};
	}
	else if (OurType == typeid(Coyote::Preset))
	{
		const Coyote::Preset *PresetObj = static_cast<const Coyote::Preset*>(Object);
		
		std::vector<msgpack::object> GotoArray;
		GotoArray.reserve(PresetObj->gotoMarks.size());
		
		for (const Coyote::PresetMark &Mark : PresetObj->gotoMarks)
		{
			GotoArray.emplace_back(PackCoyoteObject(&Mark));
		}
		
		std::vector<msgpack::object> CountdownArray;
		CountdownArray.reserve(PresetObj->countDowns.size());
		
		for (const Coyote::PresetMark &Mark : PresetObj->countDowns)
		{
			GotoArray.emplace_back(PackCoyoteObject(&Mark));
		}
		
		
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
			{ "ScrubberPosition", msgpack::object{ PresetObj->ScrubberPosition } },
			{ "InPosition", msgpack::object{ PresetObj->InPosition } },
			{ "OutPosition", msgpack::object{ PresetObj->OutPosition } },
			{ "VolumeLinked", msgpack::object{ PresetObj->VolumeLinked } },
			{ "timeCodeUpdate", msgpack::object{ PresetObj->timeCodeUpdate.GetCString() } },
			{ "tcColor", msgpack::object{ PresetObj->tcColor.GetCString() } },
			{ "gotoMarks", STLArrayToMsgpackArray(GotoArray) },
			{ "countDowns", STLArrayToMsgpackArray(CountdownArray) },
		};
	}

	assert(Values != nullptr);
	
	if (Pack)
	{
		Pack->pack(*Values);
	}
	
	return STLMapToMsgpackMap(*Values);
	
}

Coyote::BaseObject *MsgpackProc::UnpackCoyoteObject(const msgpack::object &Object, const std::type_info &Expected)
{
	Coyote::BaseObject *Result = nullptr;
	
	LDEBUG;
	
	std::map<std::string, msgpack::object> Fields;
	
	Object.convert(Fields);
	
	if 		(Expected == typeid(Coyote::Output))
	{
		LDEBUG_MSG("Debugging Output");
		
		Result = new Coyote::Output{};
		Coyote::Output *OutputObj = static_cast<Coyote::Output*>(Result);
		OutputObj->Filename = Fields["Filename"].as<std::string>();
		OutputObj->Hue = Fields["Hue"].as<int32_t>();
		OutputObj->Saturation = Fields["Saturation"].as<int32_t>();
		OutputObj->Contrast = Fields["Brightness"].as<int32_t>();
		OutputObj->MediaId = Fields["MediaId"].as<int32_t>();
		OutputObj->FadeOut = Fields["FadeOut"].as<double>();
		OutputObj->Delay = Fields["Delay"].as<double>();
		OutputObj->Active = Fields["Active"].as<int>();
		OutputObj->Audio = Fields["Audio"].as<int>();
		OutputObj->AudioChannel1 = Fields["AudioChannel1"].as<int32_t>();
		OutputObj->AudioChannel2 = Fields["AudioChannel2"].as<int32_t>();
		OutputObj->AudioChannel3 = Fields["AudioChannel3"].as<int32_t>();
		OutputObj->AudioChannel4 = Fields["AudioChannel4"].as<int32_t>();
	}
	else if (Expected == typeid(Coyote::Asset))
	{
		LDEBUG_MSG("Debugging Asset");
		Result = new Coyote::Asset{};
		Coyote::Asset *AssetObj = static_cast<Coyote::Asset*>(Result);
		
		AssetObj->FileName = Fields["FileName"].as<std::string>();
		AssetObj->NewFileName = Fields["NewFileName"].as<std::string>();
		AssetObj->CopyPercentage = Fields["CopyPercentage"].as<uint32_t>(); //Because idk if msgpack treats uint8_t like a character or a number
		AssetObj->IsReady = Fields["IsReady"].as<int>();
	}
	else if (Expected == typeid(Coyote::TimeCode))
	{
		LDEBUG_MSG("Debugging TimeCode");
		
		Result = new Coyote::TimeCode{};
		Coyote::TimeCode *TCObj = static_cast<Coyote::TimeCode*>(Result);
		
		TCObj->TRT = Fields["TRT"].as<int32_t>();
		TCObj->Time = Fields["Time"].as<int32_t>();
		TCObj->ScrubBar = Fields["ScrubBar"].as<double>();
		TCObj->Selected = Fields["Selected"].as<int>();
		TCObj->PresetKey = Fields["PresetKey"].as<int32_t>();
	}
	else if (Expected == typeid(Coyote::PresetMark))
	{
		LDEBUG_MSG("Debugging PresetMark");
		Result = new Coyote::PresetMark{};
		Coyote::PresetMark *MarkObj = static_cast<Coyote::PresetMark*>(Result);
		
		MarkObj->MarkNumber = Fields["MarkNumber"].as<std::string>();
		MarkObj->MarkName = Fields["MarkName"].as<std::string>();
		MarkObj->MarkDisplayTime = Fields["MarkDisplayTime"].as<std::string>();
		MarkObj->MarkTime = Fields["MarkTime"].as<int32_t>();
	}
		
	else if (Expected == typeid(Coyote::Preset))
	{
		LDEBUG_MSG("Debugging Preset");
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
		PresetObj->IsPlaying = Fields["IsPlaying"].as<int>();
		PresetObj->IsPaused = Fields["IsPaused"].as<int>();
		PresetObj->Selected = Fields["Selected"].as<int>();
		PresetObj->ScrubberPosition = Fields["ScrubberPosition"].as<int32_t>();
		PresetObj->InPosition = Fields["InPosition"].as<int32_t>();
		PresetObj->OutPosition = Fields["OutPosition"].as<int32_t>();
		PresetObj->VolumeLinked = Fields["VolumeLinked"].as<int>();
		PresetObj->timeCodeUpdate = Fields["timeCodeUpdate"].as<std::string>();
		PresetObj->tcColor = Fields["tcColor"].as<std::string>();
		
		//We have such sights to show you.
		PresetObj->Output1 = *std::unique_ptr<Coyote::Output>(static_cast<Coyote::Output*>(UnpackCoyoteObject(Fields["Output1"], typeid(Coyote::Output))));
		PresetObj->Output2 = *std::unique_ptr<Coyote::Output>(static_cast<Coyote::Output*>(UnpackCoyoteObject(Fields["Output2"], typeid(Coyote::Output))));
		PresetObj->Output3 = *std::unique_ptr<Coyote::Output>(static_cast<Coyote::Output*>(UnpackCoyoteObject(Fields["Output3"], typeid(Coyote::Output))));
		PresetObj->Output4 = *std::unique_ptr<Coyote::Output>(static_cast<Coyote::Output*>(UnpackCoyoteObject(Fields["Output4"], typeid(Coyote::Output))));
		
		std::vector<msgpack::object> Gotos;
		std::vector<msgpack::object> Countdowns;
		
		Fields["gotoMarks"].convert(Gotos);
		Fields["countDowns"].convert(Countdowns);
		
		
		PresetObj->gotoMarks.reserve(Gotos.size());
		PresetObj->countDowns.reserve(Countdowns.size());

		LDEBUG_MSG("Decoding Gotos");
		
		for (msgpack::object &Obj : Gotos)
		{
			const std::unique_ptr<Coyote::BaseObject> Temp { UnpackCoyoteObject(Obj, typeid(Coyote::PresetMark)) };
			
			PresetObj->gotoMarks.emplace_back(std::move(*static_cast<Coyote::PresetMark*>(Temp.get())));
		}

		LDEBUG_MSG("Decoding Countdowns");
		for (msgpack::object &Obj : Countdowns)
		{
			const std::unique_ptr<Coyote::BaseObject> Temp { UnpackCoyoteObject(Obj, typeid(Coyote::PresetMark)) };
			
			PresetObj->countDowns.emplace_back(std::move(*static_cast<Coyote::PresetMark*>(Temp.get())));
		}
		
	}
	else if (Expected == typeid(Coyote::HardwareState))
	{
		LDEBUG_MSG("Debugging HardwareState");
		
		Result = new Coyote::HardwareState{};
		Coyote::HardwareState *HWObj = static_cast<Coyote::HardwareState*>(Result);
		
		HWObj->SupportsS12G = Fields["SupportsS12G"].as<int>();
		HWObj->Resolution = ReverseResolutionMap(Fields["Resolution"].as<std::string>());
		HWObj->RefreshRate = ReverseRefreshMap(Fields["RefreshRate"].as<std::string>());
		HWObj->CurrentMode = static_cast<Coyote::HardwareMode>(Fields["CurrentMode"].as<int>());
	}
	else if (Expected == typeid(Coyote::MediaState))
	{
		LDEBUG_MSG("Debugging MediaState");
		
		Result = new Coyote::MediaState{};
		Coyote::MediaState *MSObj = static_cast<Coyote::MediaState*>(Result);
		
		std::vector<int32_t> PlayingPresets;
		std::vector<int32_t> PausedPresets;
		
		Fields["PlayingPresets"].convert(PlayingPresets);
		Fields["PausedPresets"].convert(PausedPresets);
		

		assert(MSObj->PlayingPresets.size() > PlayingPresets.size());
		assert(MSObj->PausedPresets.size() > PausedPresets.size());
		
		//Not sure msgpack.hpp can handle std::array, not gonna find out
		memcpy(MSObj->PlayingPresets.data(), PlayingPresets.data(), PlayingPresets.size() * sizeof(int32_t));
		memcpy(MSObj->PausedPresets.data(), PausedPresets.data(), PausedPresets.size() * sizeof(int32_t));
		
		MSObj->NumPresets = Fields["NumPresets"].as<int>();
		MSObj->SelectedPreset = Fields["SelectedPreset"].as<int>();
		
		std::unique_ptr<Coyote::BaseObject> TC { UnpackCoyoteObject(Fields["TimeCode"], typeid(Coyote::TimeCode)) };
		
		MSObj->Time = *static_cast<Coyote::TimeCode*>(TC.get());
	}
	else if (Expected == typeid(Coyote::Mirror))
	{
		LDEBUG_MSG("Debugging Mirror");
		
		Result = new Coyote::Mirror{};
		Coyote::Mirror *InfoObj = static_cast<Coyote::Mirror*>(Result);
		
		InfoObj->IP = Fields["IP"].as<std::string>();
		InfoObj->UnitID = Fields["UnitID"].as<std::string>();
		InfoObj->Busy = Fields["Busy"].as<int>();
		InfoObj->SupportsS12G = Fields["SupportsS12G"].as<int>();
	}
	else if (Expected == typeid(Coyote::NetworkInfo))
	{
		LDEBUG_MSG("Debugging NetworkInfo");
		
		Result = new Coyote::NetworkInfo{};
		Coyote::NetworkInfo *InfoObj = static_cast<Coyote::NetworkInfo*>(Result);
		
		InfoObj->IP = Fields["IP"].as<std::string>();
		InfoObj->Subnet = Fields["Subnet"].as<std::string>();
		InfoObj->AdapterID = Fields["AdapterID"].as<uint32_t>();
	}
	
	assert(Result != nullptr);
	
	LDEBUG_MSG("Exiting!");
	return Result;
}
