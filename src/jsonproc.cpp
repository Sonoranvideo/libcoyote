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

#include "include/internal/common.h"
#include "include/internal/jsonproc.h"
#include <json/json.h>
#include <iostream>
#include "include/libcoyote.h"
#include "include/datastructures.h"

using namespace JsonProc;
#define OUTPUT_MEMBERS	{\
							{ "Filename", Json::stringValue },\
							{ "Active", Json::booleanValue },\
							{ "Audio", Json::booleanValue },\
							{ "FadeOut", Json::realValue },\
							{ "Delay", Json::realValue },\
							{ "MediaId", Json::intValue },\
							{ "Brightness", Json::intValue },\
							{ "Contrast", Json::intValue },\
							{ "Hue", Json::intValue },\
							{ "Saturation", Json::intValue },\
						}

#define PRESET_MEMBERS	{\
							{ "PK", Json::intValue },\
							{ "Index", Json::intValue },\
							{ "Name", Json::stringValue },\
							{ "Layout", Json::stringValue },\
							{ "TRT", Json::intValue },\
							{ "Notes", Json::stringValue },\
							{ "Loop", Json::intValue },\
							{ "Link", Json::intValue },\
							{ "DisplayLink", Json::intValue },\
							{ "Selected", Json::booleanValue },\
							{ "Fade", Json::intValue },\
							{ "IsPlaying", Json::booleanValue },\
							{ "IsPaused", Json::booleanValue },\
							{ "Color", Json::stringValue },\
							{ "LeftVolume", Json::intValue },\
							{ "RightVolume", Json::intValue },\
							{ "Output1", Json::objectValue },\
							{ "Output2", Json::objectValue },\
							{ "Output3", Json::objectValue },\
							{ "Output4", Json::objectValue },\
						}

static const std::map<std::string, std::map<std::string, Json::ValueType> > CommandParameters
{
	{ "GetAssets", { { } } },
	{ "GetPresets", { { } } },
	{ "GetTimeCode", { { } } },
	{ "GetDisks", { { } } },
	{ "GetHardwareState", { { } } },
	{ "RestartService", { { } } },
	{ "BeginUpdate", { { } } },
	{ "IsUpdateDetected", { { } } },
	{ "GetIP", { { "AdapterID", Json::intValue } } },
	{ "SetIP", { { "AdapterID", Json::intValue }, { "IP", Json::stringValue }, { "Subnet", Json::stringValue } } },
	{ "Take", { { "PK", Json::intValue } } },
	{ "End", { { "PK", Json::intValue } } },
	{ "Pause", { { "PK", Json::intValue } } },
	{ "SeekTo", { { "PK", Json::intValue }, { "TimeIndex", Json::intValue } } },
	{ "InstallAsset", { { "AssetPath", Json::stringValue } } },
	{ "RenameAsset", { { "CurrentName", Json::stringValue }, { "NewName", Json::stringValue } } },
	{ "DeleteAsset", { { "AssetName", Json::stringValue } } },
	{ "CreatePreset", { PRESET_MEMBERS } },
	{ "UpdatePreset", { PRESET_MEMBERS } },
	{ "LoadPreset", { PRESET_MEMBERS } },
	{ "DeletePreset", { { "PK", Json::intValue } } },
	{ "ReorderPresets", { { "PK1", Json::intValue }, { "PK2", Json::intValue } } },
	{ "EjectDisk", { { "DriveLetter", Json::stringValue } } },
	{ "InitializeCoyote", { { "Resolution", Json::stringValue }, { "Refresh", Json::stringValue } } },
	{ "SetHardwareMode", { { "Resolution", Json::stringValue }, { "Refresh", Json::stringValue } } },
};

//Static function declarationsmms
static inline bool HasValidHeaders(const Json::Value &JsonObject);
static inline bool HasValidDataField(const Json::Value &JsonObject);

//Function definitions
Json::Value JsonProc::CreateJsonMsg(const std::string &CommandName, const std::map<std::string, Json::Value> *Values)
{
	Json::Value RetVal;
	RetVal["CommandName"] = CommandName;
	RetVal["CoyoteAPIVersion"] = COYOTE_API_VERSION;
	
	if (!Values) return RetVal;
	
	for (auto Iter = Values->begin(); Iter != Values->end(); ++Iter)
	{
		RetVal[Iter->first] = Iter->second;
	}
	
	
	
	return RetVal;
}

const std::string JsonProc::MkGetAssets(void)
{
	Json::Value Message { CreateJsonMsg("GetAssets") };
	return Message.toStyledString();
}
const std::string JsonProc::MkGetPresets(void)
{
	Json::Value Message { CreateJsonMsg("GetPresets") };
	return Message.toStyledString();
}
	
const std::string JsonProc::MkGetTimeCode(void)
{
	Json::Value Message { CreateJsonMsg("GetTimeCode") };
	return Message.toStyledString();
}
const std::string JsonProc::MkGetHardwareState(void)
{
	Json::Value Message { CreateJsonMsg("GetHardwareState") };
	return Message.toStyledString();
}
Json::Value JsonProc::CoyotePresetToJSON(const Coyote::Preset &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["Output1"] = CoyoteOutputToJSON(Ref.Output1);
	Set["Output2"] = CoyoteOutputToJSON(Ref.Output2);
	Set["Output3"] = CoyoteOutputToJSON(Ref.Output3);
	Set["Output4"] = CoyoteOutputToJSON(Ref.Output4);
	Set["Name"] = Ref.Name.GetStdString();
	Set["Layout"] = Ref.Layout.GetStdString();
	Set["Notes"] = Ref.Notes.GetStdString();
	Set["Color"] = Ref.Color.GetStdString();
	Set["PK"] = Ref.PK;
	Set["Index"] = Ref.Index;
	Set["Loop"] = Ref.Loop;
	Set["Link"] = Ref.Link;
	Set["DisplayLink"] = Ref.DisplayLink;
	Set["Fade"] = Ref.Fade;
	Set["LeftVolume"] = Ref.LeftVolume;
	Set["RightVolume"] = Ref.RightVolume;
	Set["IsPlaying"] = Ref.IsPlaying;
	Set["IsPaused"] = Ref.IsPaused;
	Set["Selected"] = Ref.Selected;
	
	return Set;
}

Json::Value CoyoteTimeCodeToJSON(const Coyote::TimeCode &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["Time"] = Ref.Time;
	Set["TRT"] = Ref.TRT;
	Set["ScrubBar"] = Ref.ScrubBar;
	
	return Set;
}

Coyote::TimeCode *JsonProc::JSONToCoyoteTimeCode(const Json::Value &Ref)
{
	auto RetVal = new Coyote::TimeCode{};
	
	RetVal->Time = Ref["Time"].asUInt();
	RetVal->TRT = Ref["TRT"].asUInt();
	RetVal->ScrubBar = Ref["ScrubBar"].asDouble();
	
	return RetVal;
}

Coyote::Preset *JsonProc::JSONToCoyotePreset(const Json::Value &Ref)
{
	auto RetVal = new Coyote::Preset{};
	
	std::unique_ptr<Coyote::Output> OutputPtr { JSONToCoyoteOutput(Ref["Output1"]) };
	
	RetVal->Output1 = std::move(*OutputPtr);
	
	OutputPtr.reset(JSONToCoyoteOutput(Ref["Output2"]));
	RetVal->Output2 = std::move(*OutputPtr);
	
	OutputPtr.reset(JSONToCoyoteOutput(Ref["Output3"]));
	RetVal->Output3 = std::move(*OutputPtr);
	
	OutputPtr.reset(JSONToCoyoteOutput(Ref["Output4"]));
	RetVal->Output4 = std::move(*OutputPtr);
	
	OutputPtr.release();

	RetVal->Name = Ref["Name"].asString();
	RetVal->Layout = Ref["Layout"].asString();
	RetVal->Notes = Ref["Notes"].asString();
	RetVal->Color = Ref["Color"].asString();
	RetVal->PK = Ref["PK"].asInt();
	RetVal->Index = Ref["Index"].asInt();
	RetVal->Loop = Ref["Loop"].asInt();
	RetVal->Link = Ref["Link"].asInt();
	RetVal->DisplayLink = Ref["DisplayLink"].asInt();
	RetVal->Fade = Ref["Fade"].asInt();
	RetVal->LeftVolume = Ref["LeftVolume"].asInt();
	RetVal->RightVolume = Ref["RightVolume"].asInt();
	RetVal->IsPlaying = Ref["IsPlaying"].asBool();
	RetVal->IsPaused = Ref["IsPaused"].asBool();
	RetVal->Selected = Ref["Selected"].asBool();
	
	return RetVal;
}

Coyote::Output *JsonProc::JSONToCoyoteOutput(const Json::Value &Ref)
{
	auto RetVal = new Coyote::Output{};
	
	
	RetVal->Filename = Ref["Filename"].asString();
	RetVal->Hue = Ref["Hue"].asInt();
	RetVal->Saturation = Ref["Saturation"].asInt();
	RetVal->Contrast = Ref["Contrast"].asInt();
	RetVal->Brightness = Ref["Brightness"].asInt();
	RetVal->MediaId = Ref["MediaId"].asInt();
	RetVal->FadeOut = Ref["FadeOut"].asDouble();
	RetVal->Delay = Ref["Delay"].asDouble();
	RetVal->Active = Ref["Active"].asBool();
	RetVal->Audio = Ref["Audio"].asBool();
	
	return RetVal;
}

Json::Value JsonProc::CoyoteOutputToJSON(const struct Coyote::Output &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["Filename"] = Ref.Filename.GetStdString();
	Set["Hue"] = Ref.Hue;
	Set["Saturation"] = Ref.Saturation;
	Set["Contrast"] = Ref.Contrast;
	Set["Brightness"] = Ref.Brightness;
	Set["MediaId"] = Ref.MediaId;
	Set["FadeOut"] = Ref.FadeOut;
	Set["Delay"] = Ref.Delay;
	Set["Active"] = Ref.Active;
	Set["Audio"] = Ref.Audio;
	
	return Set;
}

Json::Value JsonProc::CoyoteNetworkInfoToJSON(const struct Coyote::NetworkInfo &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["IP"] = Ref.IP.GetStdString();
	Set["Subnet"] = Ref.Subnet.GetStdString();
	Set["AdapterID"] = Ref.AdapterID;
	
	return Set;
}

Coyote::NetworkInfo *JsonProc::JSONToCoyoteNetworkInfo(const Json::Value &Val)
{
	auto RetVal = new Coyote::NetworkInfo{};
	
	RetVal->IP = Val["IP"].asString();
	RetVal->Subnet = Val["Subnet"].asString();
	RetVal->AdapterID = Val["AdapterID"].asInt();
	
	return RetVal;
}
Json::Value JsonProc::CoyoteMediaStateToJSON(const struct Coyote::MediaState &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["NumPresets"] = Ref.NumPresets;
	Set["Selected"] = Ref.Selected;
	
	Json::Value Playing { Json::arrayValue }, Paused { Json::arrayValue };
	
	for (size_t Inc = 0; Inc < Ref.PlayingPresets.size(); ++Inc)
	{
		if (!Ref.PlayingPresets.at(Inc)) continue;
		Playing.append(Ref.PlayingPresets.at(Inc));
	}
	Set["PlayingPresets"] = Playing;
	
	for (size_t Inc = 0; Inc < Ref.PausedPresets.size(); ++Inc)
	{
		if (!Ref.PausedPresets.at(Inc)) continue;
		Paused.append(Ref.PausedPresets.at(Inc));
	}
	
	Set["PausedPresets"] = Paused;
	
	Json::Value Time { Json::objectValue };
	
	Time["TRT"] = Ref.Time.TRT;
	Time["Time"] = Ref.Time.Time;
	Time["ScrubBar"] = Ref.Time.ScrubBar;
	
	Set["TimeCode"] = Time;
	
	return Set;
}

Coyote::MediaState *JsonProc::JSONToCoyoteMediaState(const Json::Value &Val)
{
	auto RetVal = new Coyote::MediaState{};
	
	assert(Val["PlayingPresets"].size() <= 4);
	assert(Val["PausedPresets"].size() <= 4);
	
	const Json::Value &Playing { Val["PlayingPresets"] };
	const Json::Value &Paused { Val["PausedPresets"] };
	
	//These should really be the same size.
	const size_t PlayingSize = Playing.size();
	const size_t PausedSize = Paused.size();
	
	for (size_t Inc = 0u; Inc < PlayingSize; ++Inc)
	{
		RetVal->PlayingPresets[Inc] = Playing[(Json::ArrayIndex)Inc].asUInt();
	}
	
	for (size_t Inc = 0u; Inc < PausedSize; ++Inc)
	{
		RetVal->PausedPresets[Inc] = Paused[(Json::ArrayIndex)Inc].asUInt();
	}
	
	RetVal->NumPresets = Val["NumPresets"].asInt();
	RetVal->Selected = Val["Selected"].asInt();
	
	const Json::Value &Time { Val["TimeCode"] };
	
	RetVal->Time.Time = Time["Time"].asUInt();
	RetVal->Time.TRT = Time["TRT"].asUInt();
	RetVal->Time.ScrubBar = Time["ScrubBar"].asDouble();
	
	return RetVal;
}
Json::Value JsonProc::CoyoteAssetToJSON(const struct Coyote::Asset &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["FileName"] = Ref.FileName.GetStdString();
	Set["NewFileName"] = Ref.NewFileName.GetStdString();
	Set["CopyPercentage"] = Ref.CopyPercentage;
	Set["IsReady"] = Ref.IsReady;
	
	return Set;
}

Coyote::Asset *JsonProc::JSONToCoyoteAsset(const Json::Value &Val)
{
	auto RetVal = new Coyote::Asset{};
	
	RetVal->FileName = Val["FileName"].asString();
	RetVal->NewFileName = Val["NewFileName"].asString();
	RetVal->CopyPercentage = Val["CopyPercentage"].asInt();
	RetVal->IsReady = Val["IsReady"].asBool();
	
	return RetVal;
}

Coyote::HardwareState *JsonProc::JSONToCoyoteHardwareState(const Json::Value &Value)
{
	auto RetVal = new Coyote::HardwareState{};
	
	RetVal->SupportsS12G = Value["SupportsS12G"].asBool();
	RetVal->Resolution = Value["Resolution"].asString();
	RetVal->RefreshRate = Value["RefreshRate"].asString();
	RetVal->CurrentMode = static_cast<Coyote::HardwareMode>(Value["CurrentMode"].asInt());
	return RetVal;
}

Json::Value JsonProc::CoyoteHardwareStateToJSON(const Coyote::HardwareState &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["SupportsS12G"] = Ref.SupportsS12G;
	Set["Resolution"] = Ref.Resolution.GetStdString();
	Set["RefreshRate"] = Ref.RefreshRate.GetStdString();
	Set["CurrentMode"] = Ref.CurrentMode;
	
	return Set;
}

const std::string JsonProc::JSONToServerVersion(const Json::Value &Value)
{
	assert(Value.isMember("Version"));
	
	return Value["Version"].asString();
}

static inline bool HasValidHeaders(const Json::Value &JsonObject)
{
	typedef bool (Json::Value::*MethodPtr)() const;
	
	const std::vector<std::pair<std::string, MethodPtr> > Fields
	{
		{ "CoyoteAPIVersion", &Json::Value::isString },
		{ "StatusText", &Json::Value::isString },
		{ "StatusInt", &Json::Value::isInt },
	};
	
	for (size_t Inc = 0u; Inc < Fields.size(); ++Inc)
	{
		if (!JsonObject.isMember(Fields.at(Inc).first)
			|| !(JsonObject[Fields.at(Inc).first].*(Fields.at(Inc).second))())
		{
			return false;
		}
	}
	
	return true;
}
static inline bool HasValidDataField(const Json::Value &JsonObject)
{
	return JsonObject.isObject() && JsonObject.isMember("Data") && (JsonObject["Data"].isArray() || JsonObject["Data"].isObject());
}

Coyote::StatusCode JsonProc::GetStatusCode(const Json::Value &JsonObject)
{
	return static_cast<Coyote::StatusCode>(JsonObject["StatusInt"].asInt());
}


Json::Value JsonProc::ProcessJsonMsg(const std::string &Msg)
{
	Json::Value Parsed { Json::objectValue };
	
	Json::Reader ReaderObj{};
	
	ReaderObj.parse(Msg, Parsed, false);
	
	assert(HasValidHeaders(Parsed));
	
	return Parsed;
}

Json::Value JsonProc::GetDataField(const Json::Value &Ref, const Json::Value &Default)
{
	if (!HasValidDataField(Ref)) return Default;
	
	return Ref["Data"];
}
