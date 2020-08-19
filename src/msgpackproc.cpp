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
#include "msgpackproc.h"
#include "include/libcoyote.h"
#include "include/layouts.h"

#ifdef MSGPACK_DEFAULT_API_VERSION
#undef MSGPACK_DEFAULT_API_VERSION
#endif

#define MSGPACK_DEFAULT_API_VERSION 2

#include <msgpack.hpp>

#include <typeinfo>
#include <typeindex>

//Prototypes
static bool HasValidIncomingHeaders(const std::map<std::string, msgpack::object> &Values);
static bool IsSubscriptionEvent(const std::map<std::string, msgpack::object> &Values);

extern const std::map<Coyote::ResolutionMode, std::string> ResolutionMap;
extern const std::map<Coyote::RefreshMode, std::string> RefreshMap;

Coyote::RefreshMode ReverseRefreshMap(const std::string &Lookup);
Coyote::ResolutionMode ReverseResolutionMap(const std::string &Lookup);

template <typename T>
void CoyoteGenericPack(const Coyote::Object *Obj, msgpack::object *Out, msgpack::zone &TempZone)
{
	const T *CastObj = static_cast<const T*>(Obj);
	
	msgpack::object ConvObj { *CastObj, TempZone };
	
	*Out = std::move(ConvObj);
}

template <typename T>
Coyote::Object *CoyoteGenericUnpack(const msgpack::object &Obj)
{
	T *Ptr = new T{};
	
	Obj.convert(*Ptr);
	
	return Ptr;
}

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

msgpack::object MsgpackProc::PackCoyoteObject(const Coyote::Object *Object, msgpack::zone &TempZone, msgpack::packer<msgpack::sbuffer> *Pack)
{ //I haven't used typeid/RTTI since 2014. I figured 'why not', it's here anyways.
	
	const std::type_index OurType { typeid(*Object) };

	const static std::unordered_map<std::type_index, void (*)(const Coyote::Object *Obj, msgpack::object *Out, msgpack::zone &TempZone)> Lookup
	{
		{ std::type_index(typeid(Coyote::Asset)), &CoyoteGenericPack<Coyote::Asset> },
		{ std::type_index(typeid(Coyote::ExternalAsset)), &CoyoteGenericPack<Coyote::ExternalAsset> },
		{ std::type_index(typeid(Coyote::Drive)), &CoyoteGenericPack<Coyote::Drive> },
		{ std::type_index(typeid(Coyote::AssetMetadata)), &CoyoteGenericPack<Coyote::AssetMetadata> },
		{ std::type_index(typeid(Coyote::HardwareState)), &CoyoteGenericPack<Coyote::HardwareState> },
		{ std::type_index(typeid(Coyote::TimeCode)), &CoyoteGenericPack<Coyote::TimeCode> },
		{ std::type_index(typeid(Coyote::PresetMark)), &CoyoteGenericPack<Coyote::PresetMark> },
		{ std::type_index(typeid(Coyote::TabOrdering)), &CoyoteGenericPack<Coyote::TabOrdering> },
		{ std::type_index(typeid(Coyote::MediaState)), &CoyoteGenericPack<Coyote::MediaState> },
		{ std::type_index(typeid(Coyote::NetworkInfo)), &CoyoteGenericPack<Coyote::NetworkInfo> },
		{ std::type_index(typeid(Coyote::Mirror)), &CoyoteGenericPack<Coyote::Mirror> },
		{ std::type_index(typeid(Coyote::Output)), &CoyoteGenericPack<Coyote::Output> },
		{ std::type_index(typeid(Coyote::Preset)), &CoyoteGenericPack<Coyote::Preset> },
	};

	assert(Lookup.count(OurType));
	
	msgpack::object Obj;
	
	Lookup.at(OurType)(Object, &Obj, TempZone);
	
	return Obj;
}

Coyote::Object *MsgpackProc::UnpackCoyoteObject(const msgpack::object &Obj, const std::type_info &Expected)
{
	const std::type_index OurType { Expected };
	
	LDEBUG;
	
	const static std::unordered_map<std::type_index, Coyote::Object *(*)(const msgpack::object &Obj)> Lookup
	{
		{ std::type_index(typeid(Coyote::Asset)), &CoyoteGenericUnpack<Coyote::Asset> },
		{ std::type_index(typeid(Coyote::ExternalAsset)), &CoyoteGenericUnpack<Coyote::ExternalAsset> },
		{ std::type_index(typeid(Coyote::Drive)), &CoyoteGenericUnpack<Coyote::Drive> },
		{ std::type_index(typeid(Coyote::AssetMetadata)), &CoyoteGenericUnpack<Coyote::AssetMetadata> },
		{ std::type_index(typeid(Coyote::HardwareState)), &CoyoteGenericUnpack<Coyote::HardwareState> },
		{ std::type_index(typeid(Coyote::TimeCode)), &CoyoteGenericUnpack<Coyote::TimeCode> },
		{ std::type_index(typeid(Coyote::PresetMark)), &CoyoteGenericUnpack<Coyote::PresetMark> },
		{ std::type_index(typeid(Coyote::TabOrdering)), &CoyoteGenericUnpack<Coyote::TabOrdering> },
		{ std::type_index(typeid(Coyote::MediaState)), &CoyoteGenericUnpack<Coyote::MediaState> },
		{ std::type_index(typeid(Coyote::NetworkInfo)), &CoyoteGenericUnpack<Coyote::NetworkInfo> },
		{ std::type_index(typeid(Coyote::Mirror)), &CoyoteGenericUnpack<Coyote::Mirror> },
		{ std::type_index(typeid(Coyote::Output)), &CoyoteGenericUnpack<Coyote::Output> },
		{ std::type_index(typeid(Coyote::Preset)), &CoyoteGenericUnpack<Coyote::Preset> },
	};
	
	assert(Lookup.count(OurType));
	
	Coyote::Object *Result = Lookup.at(OurType)(Obj);
	
	assert(Result != nullptr);
	
	return Result;
}
