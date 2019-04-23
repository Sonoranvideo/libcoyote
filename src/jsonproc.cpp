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
static Json::Value *ProcessJsonMsg(const std::string &Msg);
static struct Coyote::Output *JSONToCoyoteOutput(const Json::Value &Ref);
static Json::Value CoyoteOutputToJSON(const struct Coyote::Output &Ref);
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

static struct Coyote::Output *JSONToCoyoteOutput(const Json::Value &Ref)
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

static Json::Value CoyoteOutputToJSON(const struct Coyote::Output &Ref)
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

static Json::Value CoyoteAssetToJSON(const struct Coyote::Asset &Ref)
{
	Json::Value Set { Json::objectValue };
	
	Set["FileName"] = Ref.FileName;
	Set["NewFileName"] = Ref.NewFileName;
	Set["CopyPercentage"] = Ref.CopyPercentage;
	Set["IsReady"] = Ref.IsReady;
	
	return Set;
}

static struct Coyote::Asset *JSONToCoyoteAsset(const Json::Value &Val)
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

static inline Coyote::StatusCode GetStatusCode(const Json::Value &JsonObject)
{
	return static_cast<Coyote::StatusCode>(JsonObject["StatusInt"].asInt());
}

Coyote::StatusCode JsonProc::DecodeAssets(const std::string &JSON, std::vector<Coyote::Asset> &Out)
{
	std::unique_ptr<Json::Value> JsonObjPtr { ProcessJsonMsg(JSON) };
	Json::Value &JsonObj { *JsonObjPtr };
	
	const Coyote::StatusCode Status = GetStatusCode(JsonObj);
	
	if (Status != Coyote::STATUS_OK || !HasValidDataField(JsonObj))
	{
		std::cout << "Failed at decodeassets" << std::endl;
		return Status;
	}
	
	Json::Value &Data = JsonObj["Data"];
		
	for (auto Iter = Data.begin(); Iter != Data.end(); ++Iter)
	{
		std::unique_ptr<Coyote::Asset> Ptr { JSONToCoyoteAsset(*Iter) };
		
		if (Ptr)
		{
			Out.emplace_back(*Ptr);
		}
	}
	
	return Status;
}

static std::vector<Coyote::Output> *DecodeOutput(const std::string &JSON)
{
	Json::Value Parsed{};
	
	return nullptr;
}

static Json::Value *ProcessJsonMsg(const std::string &Msg)
{
	Json::Value *Parsed { new Json::Value { Json::objectValue } };
	
	Json::Reader ReaderObj{};
	
	ReaderObj.parse(Msg, *Parsed, false);
	
	assert(HasValidHeaders(*Parsed));
	
	return Parsed;
}

