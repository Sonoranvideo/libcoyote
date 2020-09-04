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

static Coyote::Object *CoyotePresetUnpack(const msgpack::object &Obj)
{ //This is a bit expensive, internal work will eventually be done to set it as an enum by default.
	Coyote::Preset *const PObj = static_cast<Coyote::Preset*>(CoyoteGenericUnpack<Coyote::Preset>(Obj));
	
	if (!PObj) return nullptr;

	std::unordered_map<std::string, msgpack::object> Mappy;
	
	Obj.convert(Mappy);
	
	std::vector<Coyote::TabOrdering> Tabs;
	
	Mappy["TabDisplayOrder"].convert(Tabs);
	
	for (Coyote::TabOrdering &Tab : Tabs)
	{
		PObj->TabDisplayOrder.emplace(Tab.TabID, std::move(Tab));
	}
	
	const std::string PresetName { Mappy["Layout"].as<std::string>() };
	
	const Coyote::LayoutInfo *const L = Coyote::LookupPresetLayoutByString(PresetName);
	
	PObj->Layout = ((L != nullptr) ? L->ID : Coyote::COYOTE_PSLAYOUT_INVALID);
	
	return PObj;
}

static void CoyotePresetPack(const Coyote::Object *const Obj, msgpack::object *const Out, msgpack::zone &TempZone)
{
	const Coyote::Preset *const PObj = static_cast<const Coyote::Preset*>(Obj);
	
	msgpack::object ConvObj { *PObj, TempZone };
	
	std::unordered_map<std::string, msgpack::object> Mappy;
	
	ConvObj.convert(Mappy);
	
	const Coyote::LayoutInfo *const Layout = Coyote::LookupPresetLayoutByID(PObj->Layout);
	
	if (Layout)
	{
		Mappy.emplace("Layout", msgpack::object{ Layout->TextName, TempZone });
	}
	
	std::vector<Coyote::TabOrdering> Tabs;
	
	Tabs.reserve(PObj->TabDisplayOrder.size());
	
	for (auto &Pair : PObj->TabDisplayOrder)
	{
		Tabs.emplace_back(std::move(Pair.second));
	}
	
	Mappy.emplace("TabDisplayOrder", msgpack::object { Tabs, TempZone });
	
	*Out = msgpack::object{ std::move(Mappy), TempZone };
}

static void CoyoteHWStatePack(const Coyote::Object *const Obj, msgpack::object *const Out, msgpack::zone &TempZone)
{
	const Coyote::HardwareState *const HWObj = static_cast<const Coyote::HardwareState*>(Obj);
	
	msgpack::object ConvObj { *HWObj, TempZone };

	std::unordered_map<std::string, msgpack::object> Mappy;
	
	ConvObj.convert(Mappy);
	
	Mappy.emplace("Resolution", msgpack::object{ ResolutionMap.at(HWObj->Resolution), TempZone });
	Mappy.emplace("RefreshRate", msgpack::object{ RefreshMap.at(HWObj->RefreshRate), TempZone });
	
	*Out = msgpack::object { std::move(Mappy), TempZone };
}

static Coyote::Object *CoyoteHWStateUnpack(const msgpack::object &Obj)
{
	Coyote::HardwareState *const HWObj = static_cast<Coyote::HardwareState*>(CoyoteGenericUnpack<Coyote::HardwareState>(Obj));
	
	if (!HWObj) return nullptr;

	std::unordered_map<std::string, msgpack::object> Mappy;
	
	Obj.convert(Mappy);
	
	HWObj->Resolution = ReverseResolutionMap(Mappy.at("Resolution").as<std::string>());
	HWObj->RefreshRate = ReverseRefreshMap(Mappy.at("RefreshRate").as<std::string>());
	
	return HWObj;
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
	
	const std::string RemoteAPIVersion { Values.at("CoyoteAPIVersion").as<std::string>() };
	
	if (RemoteAPIVersion != COYOTE_API_VERSION)
	{
		std::cerr << "libcoyote: Invalid remote API version " << RemoteAPIVersion << ", this libcoyote requires API version " COYOTE_API_VERSION " in order to function." << std::endl;
		Values["StatusInt"] = msgpack::object{Coyote::COYOTE_STATUS_NETWORKERROR, TempZone};
	}
	
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
		{ std::type_index(typeid(Coyote::HardwareState)), &CoyoteHWStatePack },
		{ std::type_index(typeid(Coyote::TimeCode)), &CoyoteGenericPack<Coyote::TimeCode> },
		{ std::type_index(typeid(Coyote::PresetMark)), &CoyoteGenericPack<Coyote::PresetMark> },
		{ std::type_index(typeid(Coyote::TabOrdering)), &CoyoteGenericPack<Coyote::TabOrdering> },
		{ std::type_index(typeid(Coyote::MediaState)), &CoyoteGenericPack<Coyote::MediaState> },
		{ std::type_index(typeid(Coyote::NetworkInfo)), &CoyoteGenericPack<Coyote::NetworkInfo> },
		{ std::type_index(typeid(Coyote::Mirror)), &CoyoteGenericPack<Coyote::Mirror> },
		{ std::type_index(typeid(Coyote::Output)), &CoyoteGenericPack<Coyote::Output> },
		{ std::type_index(typeid(Coyote::GenlockSettings)), &CoyoteGenericPack<Coyote::GenlockSettings> },
		{ std::type_index(typeid(Coyote::LANCoyote)), &CoyoteGenericPack<Coyote::LANCoyote> },
		{ std::type_index(typeid(Coyote::Preset)), &CoyotePresetPack },
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
		{ std::type_index(typeid(Coyote::HardwareState)), &CoyoteHWStateUnpack },
		{ std::type_index(typeid(Coyote::TimeCode)), &CoyoteGenericUnpack<Coyote::TimeCode> },
		{ std::type_index(typeid(Coyote::PresetMark)), &CoyoteGenericUnpack<Coyote::PresetMark> },
		{ std::type_index(typeid(Coyote::TabOrdering)), &CoyoteGenericUnpack<Coyote::TabOrdering> },
		{ std::type_index(typeid(Coyote::MediaState)), &CoyoteGenericUnpack<Coyote::MediaState> },
		{ std::type_index(typeid(Coyote::NetworkInfo)), &CoyoteGenericUnpack<Coyote::NetworkInfo> },
		{ std::type_index(typeid(Coyote::Mirror)), &CoyoteGenericUnpack<Coyote::Mirror> },
		{ std::type_index(typeid(Coyote::Output)), &CoyoteGenericUnpack<Coyote::Output> },
		{ std::type_index(typeid(Coyote::GenlockSettings)), &CoyoteGenericUnpack<Coyote::GenlockSettings> },
		{ std::type_index(typeid(Coyote::LANCoyote)), &CoyoteGenericUnpack<Coyote::LANCoyote> },
		{ std::type_index(typeid(Coyote::Preset)), &CoyotePresetUnpack },
	};
	
	assert(Lookup.count(OurType));
	
	Coyote::Object *Result = Lookup.at(OurType)(Obj);
	
	assert(Result != nullptr);
	
	return Result;
}
