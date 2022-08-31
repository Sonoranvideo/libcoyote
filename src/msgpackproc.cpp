/*
   Copyright 2022 Sonoran Video Systems

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


#include <typeinfo>
#include <typeindex>

//Prototypes
static bool HasValidIncomingHeaders(const std::unordered_map<std::string, msgpack::object> &Values);
static bool IsSubscriptionEvent(const std::unordered_map<std::string, msgpack::object> &Values);

extern const std::unordered_map<Coyote::ResolutionMode, std::string> ResolutionMap;
extern const std::unordered_map<Coyote::RefreshMode, std::string> RefreshMap;

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

static void CoyoteCanvasOrientationPack(const Coyote::Object *Obj, msgpack::object *Out, msgpack::zone &TempZone)
{ //Why is this so painful? Because C++ has a hard time representing algebraic datatypes like our video core Projector uses internally.
	const Coyote::CanvasOrientation *Orientation = static_cast<const Coyote::CanvasOrientation*>(Obj);
	using Coyote::CanvasOrientation;
	using Coyote::CanvasOrientationEnum;
	
	if (Orientation->Orientation == Coyote::CanvasOrientationEnum::Custom)
	{
		std::unordered_map<std::string, msgpack::object> Coords { { "X", msgpack::object { Orientation->CustomCoords.X } }, { "Y", msgpack::object { Orientation->CustomCoords.Y } } };
		
		std::unordered_map<std::string, msgpack::object> Mappy { { "Custom", msgpack::object(std::move(Coords), TempZone) }, };
		
		*Out = msgpack::object{ std::move(Mappy), TempZone };
	}
	
	switch (Orientation->Orientation)
	{
		case CanvasOrientationEnum::Invalid:
			*Out = msgpack::object { "Invalid", TempZone };
			break;
		case CanvasOrientationEnum::Landscape:
			*Out = msgpack::object { "Landscape", TempZone };
			break;
		case CanvasOrientationEnum::Portrait:
			*Out = msgpack::object { "Portrait", TempZone };
			break;
		case CanvasOrientationEnum::Single:
			*Out = msgpack::object { "Single", TempZone };
			break;
		case CanvasOrientationEnum::Standard:
			*Out = msgpack::object { "Standard", TempZone };
			break;
		default:
			assert(!"Bad integer value for CanvasOrientationEnum!");
	}
}

static Coyote::Object *CoyoteCanvasOrientationUnpack(const msgpack::object &Obj)
{
	using Coyote::CanvasOrientation;
	using Coyote::CanvasOrientationEnum;
	
	std::string Value;
	
	try //if it's just a string, we're not set to Custom.
	{
		Value = Obj.as<std::string>();
	}
	catch(...)
	{
		std::unordered_map<std::string, msgpack::object> Mappy;
		
		try
		{
			Obj.convert(Mappy);
		}
		catch(...)
		{ //Welp it's corrupted
			return nullptr;
		}
		
		std::unordered_map<std::string, msgpack::object> CoordsMap;

		Mappy["Custom"].convert(CoordsMap);
		
		const Coyote::Coords2D Coords { CoordsMap["X"].as<int32_t>(), CoordsMap["Y"].as<int32_t>() };
		
		return new CanvasOrientation(CanvasOrientationEnum::Custom, &Coords);
	}

	static const std::unordered_map<std::string, CanvasOrientationEnum> EnumMap
	{
		{ "Invalid", CanvasOrientationEnum::Invalid },
		{ "Landscape", CanvasOrientationEnum::Landscape },
		{ "Portrait", CanvasOrientationEnum::Portrait },
		{ "Single", CanvasOrientationEnum::Single },
		{ "Standard", CanvasOrientationEnum::Standard },
	};
	
	return new CanvasOrientation(EnumMap.at(Value));
}

static void CoyoteProjectorCanvasConfigPack(const Coyote::Object *Obj, msgpack::object *Out, msgpack::zone &TempZone)
{
	const Coyote::ProjectorCanvasConfig *Cfg = static_cast<const Coyote::ProjectorCanvasConfig*>(Obj);
	
	msgpack::object ConvObj { *Cfg, TempZone };
	
	std::unordered_map<std::string, msgpack::object> Mappy;
	
	ConvObj.convert(Mappy);
	
	msgpack::object PackObj;
	
	CoyoteCanvasOrientationPack(&Cfg->Orientation, &PackObj, TempZone);
	
	Mappy.emplace("Orientation", std::move(PackObj));
	
	*Out = msgpack::object { std::move(Mappy), TempZone };
}

static Coyote::Object *CoyoteProjectorCanvasConfigUnpack(const msgpack::object &Obj)
{
	Coyote::ProjectorCanvasConfig *Cfg = static_cast<Coyote::ProjectorCanvasConfig*>(CoyoteGenericUnpack<Coyote::ProjectorCanvasConfig>(Obj));
	
	std::unordered_map<std::string, msgpack::object> Mappy;
	
	Obj.convert(Mappy);
	
	std::unique_ptr<Coyote::CanvasOrientation> Orientation { static_cast<Coyote::CanvasOrientation*>(CoyoteCanvasOrientationUnpack(Mappy["Orientation"])) };
	
	Cfg->Orientation = std::move(*Orientation);
	
	return Cfg;
}

static Coyote::Object *CoyoteCanvasInfoUnpack(const msgpack::object &Obj)
{
	Coyote::CanvasInfo *Info = static_cast<Coyote::CanvasInfo*>(CoyoteGenericUnpack<Coyote::CanvasInfo>(Obj));

	std::unordered_map<std::string, msgpack::object> Mappy;
	
	Obj.convert(Mappy);
	
	std::unique_ptr<Coyote::ProjectorCanvasConfig> PJCfg { static_cast<Coyote::ProjectorCanvasConfig*>(CoyoteProjectorCanvasConfigUnpack(Mappy["CanvasCfg"])) };
	
	Info->CanvasCfg = std::move(*PJCfg);
	
	return Info;
}

static void CoyoteCanvasInfoPack(const Coyote::Object *Obj, msgpack::object *Out, msgpack::zone &TempZone)
{
	const Coyote::CanvasInfo *Info = static_cast<const Coyote::CanvasInfo*>(Obj);
	
	msgpack::object ConvObj { *Info, TempZone };
	
	std::unordered_map<std::string, msgpack::object> Mappy;
	
	ConvObj.convert(Mappy);
	
	msgpack::object PackObj;
	
	CoyoteProjectorCanvasConfigPack(&Info->CanvasCfg, &PackObj, TempZone);
	
	Mappy.emplace("CanvasCfg", std::move(PackObj));
	
	*Out = msgpack::object { std::move(Mappy), TempZone };
}


static Coyote::Object *CoyotePresetUnpack(const msgpack::object &Obj)
{ //This is a bit expensive, internal work will eventually be done to set it as an enum by default.
	Coyote::Preset *const PObj = static_cast<Coyote::Preset*>(CoyoteGenericUnpack<Coyote::Preset>(Obj));
	
	if (!PObj) return nullptr;

	std::unordered_map<std::string, msgpack::object> Mappy;
	
	Obj.convert(Mappy);

	std::vector<msgpack::object> RawCanvases;
	
	Mappy["Canvases"].convert(RawCanvases);
	
	std::unordered_map<std::string, int32_t> Tabs;
	
	Mappy["TabDisplayOrder"].convert(Tabs);
	
	for (const auto &Pair : Tabs)
	{
		PObj->TabDisplayOrder.emplace(Pair.first, Coyote::TabOrdering{ Pair.first, Pair.second });
	}
	
	PObj->Canvases.reserve(RawCanvases.size());
	
	for (const msgpack::object &Canvas : RawCanvases)
	{
		std::unique_ptr<const Coyote::CanvasInfo> Info { static_cast<Coyote::CanvasInfo*>(CoyoteCanvasInfoUnpack(Canvas)) };
		
		PObj->Canvases.emplace_back(std::move(*Info));
	}
	
	return PObj;
}

static void CoyotePresetPack(const Coyote::Object *const Obj, msgpack::object *const Out, msgpack::zone &TempZone)
{
	const Coyote::Preset *const PObj = static_cast<const Coyote::Preset*>(Obj);
	
	msgpack::object ConvObj { *PObj, TempZone };
	
	std::unordered_map<std::string, msgpack::object> Mappy;
	
	ConvObj.convert(Mappy);
		
	std::vector<msgpack::object> OutCanvases;
	OutCanvases.reserve(PObj->Canvases.size());
	
	for (const Coyote::CanvasInfo &Info : PObj->Canvases)
	{
		msgpack::object OutObj;
		
		CoyoteCanvasInfoPack(&Info, &OutObj, TempZone);
		
		OutCanvases.emplace_back(std::move(OutObj));
	}
	
	std::unordered_map<std::string, int32_t> Tabs;

	for (const auto &Pair : PObj->TabDisplayOrder)
	{
		Tabs.emplace(Pair.second.TabID, Pair.second.Index);
	}
	
	Mappy["Canvases"] = msgpack::object { std::move(OutCanvases), TempZone };
	Mappy["TabDisplayOrder"] = msgpack::object { std::move(Tabs), TempZone };

	*Out = msgpack::object{ std::move(Mappy), TempZone };
}

static void CoyoteHWStatePack(const Coyote::Object *const Obj, msgpack::object *const Out, msgpack::zone &TempZone)
{
	const Coyote::KonaHardwareState *const HWObj = static_cast<const Coyote::KonaHardwareState*>(Obj);
	
	msgpack::object ConvObj { *HWObj, TempZone };

	std::unordered_map<std::string, msgpack::object> Mappy;
	
	ConvObj.convert(Mappy);

	std::vector<msgpack::object> ResolutionStrings;
	
	ResolutionStrings.reserve(NUM_KONA_OUTS);
	
	for (size_t Inc = 0u; Inc < NUM_KONA_OUTS; ++Inc)
	{
		ResolutionStrings.emplace_back(msgpack::object { ResolutionMap.at(HWObj->Resolutions[Inc]).c_str() });
	}
	
	Mappy.emplace("Resolutions", msgpack::object{ MsgpackProc::STLArrayToMsgpackArray(ResolutionStrings, TempZone), TempZone });
	Mappy.emplace("RefreshRate", msgpack::object{ RefreshMap.at(HWObj->RefreshRate), TempZone });
	Mappy.emplace("AudioConfig", msgpack::object{ (int)HWObj->AudioConfig, TempZone });
	
	*Out = msgpack::object { std::move(Mappy), TempZone };
}

static Coyote::Object *CoyoteHWStateUnpack(const msgpack::object &Obj)
{
	Coyote::KonaHardwareState *const HWObj = static_cast<Coyote::KonaHardwareState*>(CoyoteGenericUnpack<Coyote::KonaHardwareState>(Obj));
	
	if (!HWObj) return nullptr;

	std::unordered_map<std::string, msgpack::object> Mappy;
	
	Obj.convert(Mappy);
	
	const auto ResStrings = Mappy.at("Resolutions").as<std::vector<std::string>>();
	
	const size_t NumOuts = std::min(ResStrings.size(), (size_t)NUM_KONA_OUTS);
	
	for (size_t Inc = 0; Inc < NumOuts; ++Inc)
	{
		HWObj->Resolutions.at(Inc) = ReverseResolutionMap(ResStrings.at(Inc));
	}
	
	HWObj->RefreshRate = ReverseRefreshMap(Mappy.at("RefreshRate").as<std::string>());
	HWObj->AudioConfig = (Coyote::KonaAudioConfig)Mappy.at("AudioConfig").as<int>();
	
	return HWObj;
}
	
//Definitions
void MsgpackProc::InitOutgoingMsg(msgpack::packer<msgpack::sbuffer> &Pack, const std::string &CommandName, const uint64_t MsgID, const msgpack::object *Values)
{
	msgpack::zone TempZone;
	
	std::unordered_map<std::string, msgpack::object> TotalValues
	{
		{ "CommandName", msgpack::object{ CommandName.c_str(), TempZone} },
		{ "CoyoteAPIVersion", msgpack::object{ COYOTE_API_VERSION, TempZone } },
	};
	
	if (MsgID) TotalValues.emplace("MsgID", msgpack::object{ MsgID });
	
	if (Values != nullptr) TotalValues["Data"] = *Values;

	Pack.pack(TotalValues);
}

static bool HasValidIncomingHeaders(const std::unordered_map<std::string, msgpack::object> &Values)
{
	static const char *const Required[] = { "CommandName", "CoyoteAPIVersion", "StatusInt", "StatusText" };
	static const size_t NumFields = sizeof Required / sizeof *Required;
	
	for (size_t Inc = 0u; Inc < NumFields; ++Inc)
	{
		if (!Values.count(Required[Inc])) return false;
	}
	
	return true;
}

static bool IsSubscriptionEvent(const std::unordered_map<std::string, msgpack::object> &Values)
{
	return Values.count("SubscriptionEvent");
}

std::unordered_map<std::string, msgpack::object> MsgpackProc::InitIncomingMsg(const void *Data, const size_t DataLength, msgpack::zone &TempZone, uint64_t *MsgIDOut)
{
	//Unpack binary data.
	msgpack::unpacked Result;

	const msgpack::object &Object { msgpack::unpack(TempZone, (const char*)Data, DataLength) };
	
	//Convert into a map of smaller msgpack objects
	std::unordered_map<std::string, msgpack::object> Values;
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
		{ std::type_index(typeid(Coyote::KonaHardwareState)), &CoyoteHWStatePack },
		{ std::type_index(typeid(Coyote::TimeCode)), &CoyoteGenericPack<Coyote::TimeCode> },
		{ std::type_index(typeid(Coyote::PresetMark)), &CoyoteGenericPack<Coyote::PresetMark> },
		{ std::type_index(typeid(Coyote::NetworkInfo)), &CoyoteGenericPack<Coyote::NetworkInfo> },
		{ std::type_index(typeid(Coyote::Mirror)), &CoyoteGenericPack<Coyote::Mirror> },
		{ std::type_index(typeid(Coyote::GenlockSettings)), &CoyoteGenericPack<Coyote::GenlockSettings> },
		{ std::type_index(typeid(Coyote::LANCoyote)), &CoyoteGenericPack<Coyote::LANCoyote> },
		{ std::type_index(typeid(Coyote::PresetState)), &CoyoteGenericPack<Coyote::PresetState> },
		{ std::type_index(typeid(Coyote::Preset)), &CoyotePresetPack },
		{ std::type_index(typeid(Coyote::CanvasOrientation)), &CoyoteCanvasOrientationPack },
		{ std::type_index(typeid(Coyote::PinnedAsset)), &CoyoteGenericPack<Coyote::PinnedAsset> },
		{ std::type_index(typeid(Coyote::CanvasInfo)), &CoyoteCanvasInfoPack },
		{ std::type_index(typeid(Coyote::ProjectorCanvasConfig)), &CoyoteProjectorCanvasConfigPack },
	};

	assert(Lookup.count(OurType));
	
	msgpack::object Obj;
	
	Lookup.at(OurType)(Object, &Obj, TempZone);
	
	return Obj;
}

Coyote::Object *MsgpackProc::UnpackCoyoteObject(const msgpack::object &Obj, const std::type_info &Expected)
{
	const std::type_index OurType { Expected };
	
	LDEBUG_MSG("Got incoming object " << Obj);
	
	const static std::unordered_map<std::type_index, Coyote::Object *(*)(const msgpack::object &Obj)> Lookup
	{
		{ std::type_index(typeid(Coyote::Asset)), &CoyoteGenericUnpack<Coyote::Asset> },
		{ std::type_index(typeid(Coyote::ExternalAsset)), &CoyoteGenericUnpack<Coyote::ExternalAsset> },
		{ std::type_index(typeid(Coyote::Drive)), &CoyoteGenericUnpack<Coyote::Drive> },
		{ std::type_index(typeid(Coyote::AssetMetadata)), &CoyoteGenericUnpack<Coyote::AssetMetadata> },
		{ std::type_index(typeid(Coyote::KonaHardwareState)), &CoyoteHWStateUnpack },
		{ std::type_index(typeid(Coyote::TimeCode)), &CoyoteGenericUnpack<Coyote::TimeCode> },
		{ std::type_index(typeid(Coyote::PresetMark)), &CoyoteGenericUnpack<Coyote::PresetMark> },
		//~ { std::type_index(typeid(Coyote::TabOrdering)), &CoyoteGenericUnpack<Coyote::TabOrdering> }, Not in here for a reason
		{ std::type_index(typeid(Coyote::NetworkInfo)), &CoyoteGenericUnpack<Coyote::NetworkInfo> },
		{ std::type_index(typeid(Coyote::Mirror)), &CoyoteGenericUnpack<Coyote::Mirror> },
		{ std::type_index(typeid(Coyote::GenlockSettings)), &CoyoteGenericUnpack<Coyote::GenlockSettings> },
		{ std::type_index(typeid(Coyote::LANCoyote)), &CoyoteGenericUnpack<Coyote::LANCoyote> },
		{ std::type_index(typeid(Coyote::PresetState)), &CoyoteGenericUnpack<Coyote::PresetState> },
		{ std::type_index(typeid(Coyote::Preset)), &CoyotePresetUnpack },
		{ std::type_index(typeid(Coyote::CanvasOrientation)), &CoyoteCanvasOrientationUnpack },
		{ std::type_index(typeid(Coyote::PinnedAsset)), &CoyoteGenericUnpack<Coyote::PinnedAsset> },
		{ std::type_index(typeid(Coyote::CanvasInfo)), &CoyoteCanvasInfoUnpack },
		{ std::type_index(typeid(Coyote::ProjectorCanvasConfig)), &CoyoteProjectorCanvasConfigUnpack },
	};
	
	assert(Lookup.count(OurType));
	
	Coyote::Object *Result = Lookup.at(OurType)(Obj);
	
	assert(Result != nullptr);
	
	return Result;
}
