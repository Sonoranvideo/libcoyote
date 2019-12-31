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
#include "common.h"
#include "msgpackproc.h"
#include "include/libcoyote.h"
#include "include/layouts.h"

#ifdef MSGPACK_DEFAULT_API_VERSION
#undef MSGPACK_DEFAULT_API_VERSION
#endif

#define MSGPACK_DEFAULT_API_VERSION 2

#include <msgpack.hpp>

#include <typeinfo>

//Prototypes
static bool HasValidIncomingHeaders(const std::map<std::string, msgpack::object> &Values);
static bool IsSubscriptionEvent(const std::map<std::string, msgpack::object> &Values);

extern const std::map<Coyote::ResolutionMode, std::string> ResolutionMap;
extern const std::map<Coyote::RefreshMode, std::string> RefreshMap;

Coyote::RefreshMode ReverseRefreshMap(const std::string &Lookup);
Coyote::ResolutionMode ReverseResolutionMap(const std::string &Lookup);

//Definitions
void MsgpackProc::InitOutgoingMsg(msgpack::packer<msgpack::sbuffer> &Pack, const std::string &CommandName, const uint64_t MsgID, const msgpack::object *Values)
{
	msgpack::zone TempZone;
	
	std::map<std::string, msgpack::object> TotalValues
	{
		{ "CommandName", msgpack::object{ CommandName.c_str(), TempZone} },
		{ "CoyoteAPIVersion", msgpack::object{ COYOTE_API_VERSION, TempZone } },
	};
	
	if (MsgID) TotalValues.emplace("MsgID", msgpack::object{ MsgID });
	
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

std::map<std::string, msgpack::object> MsgpackProc::InitIncomingMsg(const void *Data, const size_t DataLength, msgpack::zone &TempZone, uint64_t *MsgIDOut)
{
	//Unpack binary data.
	msgpack::unpacked Result;

	const msgpack::object &Object { msgpack::unpack(TempZone, (const char*)Data, DataLength) };
	
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

msgpack::object MsgpackProc::PackCoyoteObject(const Coyote::BaseObject *Object, msgpack::zone &TempZone, msgpack::packer<msgpack::sbuffer> *Pack)
{ //I haven't used typeid/RTTI since 2014. I figured 'why not', it's here anyways.
	
	const std::type_info &OurType { typeid(*Object) };

	std::map<std::string, msgpack::object> *Values = nullptr;
	
	if 		(OurType == typeid(Coyote::Asset))
	{
		const Coyote::Asset *AssetObj = static_cast<const Coyote::Asset*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "FileName", msgpack::object{ AssetObj->FileName.GetCString(), TempZone } },
			{ "NewFileName", msgpack::object{ AssetObj->NewFileName.GetCString(), TempZone } },
			{ "CopyPercentage", msgpack::object{ (int)AssetObj->CopyPercentage, TempZone } },
			{ "IsReady", msgpack::object{ AssetObj->IsReady, TempZone } },
			{ "AudioSampleRate", msgpack::object { AssetObj->AudioSampleRate, TempZone } },
			{ "AudioNumChannels", msgpack::object { AssetObj->AudioNumChannels, TempZone } },
			{ "DurationMs", msgpack::object { AssetObj->DurationMs, TempZone } },
			{ "Size", msgpack::object { AssetObj->Size, TempZone } },
			{ "VideoFrameRate", msgpack::object { AssetObj->VideoFrameRate, TempZone } },
			{ "VideoHeight", msgpack::object { AssetObj->VideoHeight, TempZone } },
			{ "VideoWidth", msgpack::object { AssetObj->VideoWidth, TempZone } },
			{ "Videoencoding_FCC", msgpack::object { AssetObj->Videoencoding_FCC.GetCString(), TempZone } },
			{ "Audioencoding_FCC", msgpack::object { AssetObj->Audioencoding_FCC.GetCString(), TempZone } },
			{ "SupportedVidEncode", msgpack::object { AssetObj->SupportedVidEncode, TempZone } },
			{ "SupportedAudEncode", msgpack::object { AssetObj->SupportedAudEncode, TempZone } },
		};
	}
	else if (OurType == typeid(Coyote::HardwareState))
	{
		const Coyote::HardwareState *HWStateObj = static_cast<const Coyote::HardwareState*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "SupportsS12G", msgpack::object{ HWStateObj->SupportsS12G, TempZone } },
			{ "Resolution", msgpack::object{ ResolutionMap.at(HWStateObj->Resolution).c_str(), TempZone } },
			{ "RefreshRate", msgpack::object{ RefreshMap.at(HWStateObj->RefreshRate).c_str(), TempZone } },
			{ "HDRMode", msgpack::object{ static_cast<int>(HWStateObj->HDRMode), TempZone } },
			{ "EOTFSetting", msgpack::object{ static_cast<int>(HWStateObj->EOTFSetting), TempZone } },
			{ "ConstLumin", msgpack::object{ static_cast<int>(HWStateObj->ConstLumin), TempZone } },
			{ "CurrentMode", msgpack::object{ static_cast<int>(HWStateObj->CurrentMode), TempZone } },
		};
	}
	else if (OurType == typeid(Coyote::TimeCode))
	{
		const Coyote::TimeCode *TCObj = static_cast<const Coyote::TimeCode*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "TRT", msgpack::object{ TCObj->TRT, TempZone } },
			{ "Time", msgpack::object{ TCObj->Time, TempZone } },
			{ "ScrubBar", msgpack::object{ TCObj->ScrubBar, TempZone } },
			{ "Selected", msgpack::object{ TCObj->Selected, TempZone } },
			{ "PresetKey", msgpack::object{ TCObj->PresetKey, TempZone } },
			{ "LeftChannelVolume", msgpack::object { TCObj->LeftChannelVolume, TempZone } },
			{ "RightChannelVolume", msgpack::object { TCObj->RightChannelVolume, TempZone } },
			{ "Player1LeftVolume", msgpack::object { TCObj->Player1LeftVolume, TempZone } },
			{ "Player1RightVolume", msgpack::object { TCObj->Player1RightVolume, TempZone } },
			{ "Player2LeftVolume", msgpack::object { TCObj->Player2LeftVolume, TempZone } },
			{ "Player2RightVolume", msgpack::object { TCObj->Player2RightVolume, TempZone } },
			{ "Player3LeftVolume", msgpack::object { TCObj->Player3LeftVolume, TempZone } },
			{ "Player3RightVolume", msgpack::object { TCObj->Player3RightVolume, TempZone } },
			{ "Player4LeftVolume", msgpack::object { TCObj->Player4LeftVolume, TempZone } },
			{ "Player4RightVolume", msgpack::object { TCObj->Player4RightVolume, TempZone } },
		};
	}
	else if (OurType == typeid(Coyote::PresetMark))
	{
		const Coyote::PresetMark *MarkObj = static_cast<const Coyote::PresetMark*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "MarkNumber", msgpack::object { MarkObj->MarkNumber.GetCString(), TempZone } },
			{ "MarkName", msgpack::object { MarkObj->MarkName.GetCString(), TempZone } },
			{ "MarkDisplayTime", msgpack::object { MarkObj->MarkDisplayTime.GetCString(), TempZone } },
			{ "MarkTime", msgpack::object { MarkObj->MarkTime, TempZone } },
			
		};
	}
	else if (OurType == typeid(Coyote::MediaState))
	{
		const Coyote::MediaState *MediaStateObj = static_cast<const Coyote::MediaState*>(Object);

		std::vector<msgpack::object> TCs;
		
		for (const Coyote::TimeCode &TC : MediaStateObj->TimeCodes)
		{
			TCs.emplace_back(PackCoyoteObject(&TC, TempZone));
		}
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "NumPresets", msgpack::object{ MediaStateObj->NumPresets, TempZone } },
			{ "SelectedPreset", msgpack::object{ MediaStateObj->SelectedPreset, TempZone } },
			{ "PlayingPresets", msgpack::object{ STLArrayToMsgpackArray(MediaStateObj->PlayingPresets, TempZone) } },
			{ "PausedPresets", msgpack::object{ STLArrayToMsgpackArray(MediaStateObj->PausedPresets, TempZone) } },
			{ "TimeCodes", msgpack::object{ STLArrayToMsgpackArray(TCs, TempZone) } },
		};
	}
	else if (OurType == typeid(Coyote::NetworkInfo))
	{
		const Coyote::NetworkInfo *NetInfo = static_cast<const Coyote::NetworkInfo*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "IP", msgpack::object{ NetInfo->IP.GetCString(), TempZone } },
			{ "Subnet", msgpack::object{ NetInfo->Subnet.GetCString(), TempZone } },
			{ "AdapterID", msgpack::object{ NetInfo->AdapterID, TempZone } },
		};
	}
	else if (OurType == typeid(Coyote::Mirror))
	{
		const Coyote::Mirror *MirrorInfo = static_cast<const Coyote::Mirror*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "IP", msgpack::object{ MirrorInfo->IP.GetCString(), TempZone } },
			{ "Busy", msgpack::object{ MirrorInfo->Busy, TempZone } },
			{ "SupportsS12G", msgpack::object{ MirrorInfo->SupportsS12G, TempZone } },
			{ "UnitID", msgpack::object{ MirrorInfo->UnitID.GetCString(), TempZone } },
		};
	}
	else if (OurType == typeid(Coyote::Output))
	{
		const Coyote::Output *OutputObj = static_cast<const Coyote::Output*>(Object);
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "Filename", msgpack::object{ OutputObj->Filename.GetCString(), TempZone } },
			{ "Hue", msgpack::object{ OutputObj->Hue, TempZone } },
			{ "Saturation", msgpack::object{ OutputObj->Saturation, TempZone } },
			{ "Contrast", msgpack::object{ OutputObj->Contrast, TempZone } },
			{ "Brightness", msgpack::object{ OutputObj->Brightness, TempZone } },
			{ "MediaId", msgpack::object{ OutputObj->MediaId, TempZone } },
			{ "FadeOut", msgpack::object{ OutputObj->FadeOut, TempZone } },
			{ "Delay", msgpack::object{ OutputObj->Delay, TempZone } },
			{ "Active", msgpack::object{ OutputObj->Active, TempZone } },
			{ "Audio", msgpack::object{ OutputObj->Audio, TempZone } },
			{ "AudioChannel1", msgpack::object{ OutputObj->AudioChannel1, TempZone } },
			{ "AudioChannel2", msgpack::object{ OutputObj->AudioChannel2, TempZone } },
			{ "AudioChannel3", msgpack::object{ OutputObj->AudioChannel3, TempZone } },
			{ "AudioChannel4", msgpack::object{ OutputObj->AudioChannel4, TempZone } },
			{ "OriginalHeight", msgpack::object { OutputObj->OriginalHeight, TempZone } },
			{ "OriginalWidth", msgpack::object { OutputObj->OriginalWidth, TempZone } },
			{ "CustomDestX", msgpack::object { OutputObj->CustomDestX, TempZone } },
			{ "CustomDestY", msgpack::object { OutputObj->CustomDestY, TempZone } },
			{ "CustHeight", msgpack::object { OutputObj->CustHeight, TempZone } },
			{ "CustWidth", msgpack::object { OutputObj->CustWidth, TempZone } },
			{ "HorizontalCrop", msgpack::object { OutputObj->HorizontalCrop, TempZone } },
			{ "VerticalCrop", msgpack::object { OutputObj->VerticalCrop, TempZone } },
			{ "JustifyTop", msgpack::object { OutputObj->JustifyTop, TempZone } },
			{ "JustifyBottom", msgpack::object { OutputObj->JustifyBottom, TempZone } },
			{ "JustifyRight", msgpack::object { OutputObj->JustifyRight, TempZone } },
			{ "JustifyLeft", msgpack::object { OutputObj->JustifyLeft, TempZone } },
			{ "CenterVideo", msgpack::object { OutputObj->CenterVideo, TempZone } },
			{ "NativeSize", msgpack::object { OutputObj->NativeSize, TempZone } },
			{ "LetterPillarBox", msgpack::object { OutputObj->LetterPillarBox, TempZone } },
			{ "TempFlag", msgpack::object { OutputObj->TempFlag, TempZone } },
			{ "Anamorphic", msgpack::object { OutputObj->Anamorphic, TempZone } },
			{ "MultiviewAudio", msgpack::object { OutputObj->MultiviewAudio, TempZone } },
			{ "EnableTimeCode", msgpack::object { OutputObj->EnableTimeCode, TempZone } },
		};
	}
	else if (OurType == typeid(Coyote::Preset))
	{
		const Coyote::Preset *PresetObj = static_cast<const Coyote::Preset*>(Object);
		
		std::vector<msgpack::object> GotoArray;
		GotoArray.reserve(PresetObj->gotoMarks.size());
		
		for (const Coyote::PresetMark &Mark : PresetObj->gotoMarks)
		{
			GotoArray.emplace_back(PackCoyoteObject(&Mark, TempZone));
		}
		
		std::vector<msgpack::object> CountdownArray;
		CountdownArray.reserve(PresetObj->countDowns.size());
		
		for (const Coyote::PresetMark &Mark : PresetObj->countDowns)
		{
			GotoArray.emplace_back(PackCoyoteObject(&Mark, TempZone));
		}
		
		
		Values = new std::map<std::string, msgpack::object>
		{
			{ "Output1", msgpack::object{ PackCoyoteObject(&PresetObj->Output1, TempZone) } },
			{ "Output2", msgpack::object{ PackCoyoteObject(&PresetObj->Output2, TempZone) } },
			{ "Output3", msgpack::object{ PackCoyoteObject(&PresetObj->Output3, TempZone) } },
			{ "Output4", msgpack::object{ PackCoyoteObject(&PresetObj->Output4, TempZone) } },
			{ "Name", msgpack::object{ PresetObj->Name.GetCString(), TempZone } },
			{ "Layout", msgpack::object{ LookupPresetLayoutByID(PresetObj->Layout)->TextName.c_str(), TempZone } },
			{ "Notes", msgpack::object{ PresetObj->Notes.GetCString(), TempZone } },
			{ "Color", msgpack::object{ PresetObj->Color.GetCString(), TempZone } },
			{ "PK", msgpack::object{ PresetObj->PK, TempZone } },
			{ "TRT", msgpack::object{ PresetObj->TRT, TempZone } },
			{ "Index", msgpack::object{ PresetObj->Index, TempZone } },
			{ "Loop", msgpack::object{ PresetObj->Loop, TempZone } },
			{ "Link", msgpack::object{ PresetObj->Link, TempZone } },
			{ "DisplayLink", msgpack::object{ PresetObj->DisplayLink, TempZone } },
			{ "Fade", msgpack::object{ PresetObj->Fade, TempZone } },
			{ "LeftVolume", msgpack::object{ PresetObj->LeftVolume, TempZone } },
			{ "RightVolume", msgpack::object{ PresetObj->RightVolume, TempZone } },
			{ "IsPlaying", msgpack::object{ PresetObj->IsPlaying, TempZone } },
			{ "IsPaused", msgpack::object{ PresetObj->IsPaused, TempZone } },
			{ "Selected", msgpack::object{ PresetObj->Selected, TempZone } },
			{ "ScrubberPosition", msgpack::object{ PresetObj->ScrubberPosition, TempZone } },
			{ "InPosition", msgpack::object{ PresetObj->InPosition, TempZone } },
			{ "OutPosition", msgpack::object{ PresetObj->OutPosition, TempZone } },
			{ "VolumeLinked", msgpack::object{ PresetObj->VolumeLinked, TempZone } },
			{ "timeCodeUpdate", msgpack::object{ PresetObj->timeCodeUpdate.GetCString(), TempZone } },
			{ "FreezeAtEnd", msgpack::object{ PresetObj->FreezeAtEnd, TempZone } },
			{ "DisplayOrderIndex", msgpack::object{ PresetObj->DisplayOrderIndex, TempZone } },
			{ "Dissolve", msgpack::object{ PresetObj->Dissolve, TempZone } },
			{ "tcColor", msgpack::object{ PresetObj->tcColor.GetCString(), TempZone } },
			{ "gotoMarks", STLArrayToMsgpackArray(GotoArray, TempZone) },
			{ "countDowns", STLArrayToMsgpackArray(CountdownArray, TempZone) },
		};
	}

	assert(Values != nullptr);
	
	if (Pack)
	{
		Pack->pack(*Values);
	}
	
	return STLMapToMsgpackMap(*Values, TempZone);
	
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
		OutputObj->EnableTimeCode = Fields["EnableTimeCode"].as<int>();
		OutputObj->JustifyTop = Fields["JustifyTop"].as<int>();
		OutputObj->JustifyBottom = Fields["JustifyBottom"].as<int>();
		OutputObj->JustifyLeft = Fields["JustifyLeft"].as<int>();
		OutputObj->JustifyRight = Fields["JustifyRight"].as<int>();
		OutputObj->CenterVideo = Fields["CenterVideo"].as<int>();
		OutputObj->NativeSize = Fields["NativeSize"].as<int>();
		OutputObj->LetterPillarBox = Fields["LetterPillarBox"].as<int>();
		OutputObj->TempFlag = Fields["TempFlag"].as<int>();
		OutputObj->Anamorphic = Fields["Anamorphic"].as<int>();
		OutputObj->MultiviewAudio = Fields["MultiviewAudio"].as<int>();
		OutputObj->AudioChannel1 = Fields["AudioChannel1"].as<int32_t>();
		OutputObj->AudioChannel2 = Fields["AudioChannel2"].as<int32_t>();
		OutputObj->AudioChannel3 = Fields["AudioChannel3"].as<int32_t>();
		OutputObj->AudioChannel4 = Fields["AudioChannel4"].as<int32_t>();
		OutputObj->OriginalHeight = Fields["OriginalHeight"].as<int32_t>();
		OutputObj->OriginalWidth = Fields["OriginalWidth"].as<int32_t>();
		OutputObj->CustomDestX = Fields["CustomDestX"].as<int32_t>();
		OutputObj->CustomDestY = Fields["CustomDestY"].as<int32_t>();
		OutputObj->CustHeight = Fields["CustHeight"].as<int32_t>();
		OutputObj->CustWidth = Fields["CustWidth"].as<int32_t>();
		OutputObj->HorizontalCrop = Fields["HorizontalCrop"].as<int32_t>();
		OutputObj->VerticalCrop = Fields["VerticalCrop"].as<int32_t>();
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
		AssetObj->AudioSampleRate = Fields["AudioSampleRate"].as<int32_t>();
		AssetObj->AudioNumChannels = Fields["AudioNumChannels"].as<int32_t>();
		AssetObj->DurationMs = Fields["DurationMs"].as<int32_t>();
		AssetObj->Size = Fields["Size"].as<int32_t>();
		AssetObj->VideoFrameRate = Fields["VideoFrameRate"].as<int32_t>();
		AssetObj->VideoHeight = Fields["VideoHeight"].as<int32_t>();
		AssetObj->VideoWidth = Fields["VideoWidth"].as<int32_t>();
		AssetObj->Videoencoding_FCC = Fields["Videoencoding_FCC"].as<std::string>();
		AssetObj->Audioencoding_FCC = Fields["Audioencoding_FCC"].as<std::string>();
		AssetObj->SupportedVidEncode = Fields["SupportedVidEncode"].as<int>();
		AssetObj->SupportedAudEncode = Fields["SupportedAudEncode"].as<int>();
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
		TCObj->LeftChannelVolume = Fields["LeftChannelVolume"].as<int32_t>();
		TCObj->RightChannelVolume = Fields["LeftChannelVolume"].as<int32_t>();
		TCObj->Player1LeftVolume = Fields["Player1LeftVolume"].as<int32_t>();
		TCObj->Player1RightVolume = Fields["Player1RightVolume"].as<int32_t>();
		TCObj->Player2LeftVolume = Fields["Player2LeftVolume"].as<int32_t>();
		TCObj->Player2RightVolume = Fields["Player2RightVolume"].as<int32_t>();
		TCObj->Player3LeftVolume = Fields["Player3LeftVolume"].as<int32_t>();
		TCObj->Player3RightVolume = Fields["Player3RightVolume"].as<int32_t>();
		TCObj->Player4LeftVolume = Fields["Player4LeftVolume"].as<int32_t>();
		TCObj->Player4RightVolume = Fields["Player4RightVolume"].as<int32_t>();
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
		PresetObj->Layout = Coyote::LookupPresetLayoutByString(Fields["Layout"].as<std::string>())->ID;
		PresetObj->Notes = Fields["Notes"].as<std::string>();
		PresetObj->Color = Fields["Color"].as<std::string>();
		PresetObj->PK = Fields["PK"].as<int>();
		PresetObj->TRT = Fields["TRT"].as<int>();
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
		PresetObj->FreezeAtEnd = Fields["FreezeAtEnd"].as<int>();
		PresetObj->DisplayOrderIndex = Fields["DisplayOrderIndex"].as<int32_t>();
		PresetObj->Dissolve = Fields["Dissolve"].as<int32_t>();
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
		HWObj->ConstLumin = Fields["ConstLumin"].as<int>();
		HWObj->Resolution = ReverseResolutionMap(Fields["Resolution"].as<std::string>());
		HWObj->RefreshRate = ReverseRefreshMap(Fields["RefreshRate"].as<std::string>());
		HWObj->CurrentMode = static_cast<Coyote::HardwareMode>(Fields["CurrentMode"].as<int>());
		HWObj->HDRMode = static_cast<Coyote::HDRMode>(Fields["HDRMode"].as<int>());
		HWObj->EOTFSetting = static_cast<Coyote::EOTFMode>(Fields["EOTFSetting"].as<int>());
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
		
		std::list<msgpack::object> TimeCodeObjects;
		
		Fields["TimeCodes"].convert(TimeCodeObjects);
		
		uint8_t Counter = 0;
		for (msgpack::object &Obj : TimeCodeObjects)
		{
			std::unique_ptr<Coyote::BaseObject> TC { UnpackCoyoteObject(Obj, typeid(Coyote::TimeCode)) };
			MSObj->TimeCodes.at(Counter++) = std::move(*static_cast<Coyote::TimeCode*>(TC.get()));
		}

		assert(Counter < COYOTE_MAX_OUTPUTS + 1);
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
