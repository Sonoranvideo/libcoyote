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
#include "jsonproc.h"
#include <json/json.h>
#include <iostream>
#include "libcoyote.h"
#include "datastructures.h"

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
};

//Static function declarationsmms
static struct Coyote::Output *JSONToCoyoteOutput(const Json::Value &Ref);
static Json::Value CoyoteAssetToJSON(const struct Coyote::Asset &Ref);
static struct Coyote::Asset *JSONToCoyoteAsset(const Json::Value &Val);
static inline Coyote::StatusCode GetStatusCode(const Json::Value &JsonObject);
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
Json::Value CoyotePresetToJSON(const Coyote::Preset &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["Output1"] = CoyoteOutputToJSON(Ref.Output1);
	Set["Output2"] = CoyoteOutputToJSON(Ref.Output2);
	Set["Output3"] = CoyoteOutputToJSON(Ref.Output3);
	Set["Output4"] = CoyoteOutputToJSON(Ref.Output4);
	Set["Name"] = Ref.Name;
	Set["Layout"] = Ref.Layout;
	Set["Notes"] = Ref.Notes;
	Set["Color"] = Ref.Color;
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
	
	Set["Filename"] = Ref.Filename;
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

Json::Value JsonProc::CoyoteAssetToJSON(const struct Coyote::Asset &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["FileName"] = Ref.FileName;
	Set["NewFileName"] = Ref.NewFileName;
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
	return JsonObject.isMember("Data") && JsonObject["Data"].isArray();
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

Json::Value JsonProc::GetDataField(const Json::Value &Ref)
{
	assert(HasValidDataField(Ref));
	
	return Ref["Data"];
}
