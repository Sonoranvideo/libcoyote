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
#include "../src/include/session.h"
#include "pybind11/include/pybind11/pybind11.h"
#include "pybind11/include/pybind11/stl.h"
#include "pybind11/include/pybind11/stl_bind.h"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<std::string>)
PYBIND11_MAKE_OPAQUE(std::vector<Coyote::Asset>)
PYBIND11_MAKE_OPAQUE(std::vector<Coyote::Preset>)

#define ACLASSF(a, b) .def(#b, &Coyote::a::b)
#define ACLASSD(a, b) .def_readwrite(#b, &Coyote::a::b)
#define ACLASSBD(a, b) .def_readwrite(#b, &a::b)

#define EMEMDEF(a) .value(#a, Coyote::a)

PYBIND11_MODULE(pycoyote, ModObj)
{
	py::class_<Coyote::BaseObject>(ModObj, "BaseObject")
	.def(py::init<>());
	
	py::class_<Coyote::CoyoteString>(ModObj, "CoyoteString")
	.def(py::init<const std::string &>())
	.def("__str__", &Coyote::CoyoteString::operator std::string)
	.def("__repr__", &Coyote::CoyoteString::operator std::string)
	ACLASSF(CoyoteString, Set)
	ACLASSF(CoyoteString, GetCString)
	ACLASSF(CoyoteString, GetStdString)
	ACLASSF(CoyoteString, GetLength);
	
	py::bind_vector<std::vector<Coyote::Preset> >(ModObj, "PresetList");
	py::bind_vector<std::vector<Coyote::Asset> >(ModObj, "AssetList");
	py::bind_vector<std::vector<std::string> >(ModObj, "StringList");
	py::bind_vector<std::vector<Coyote::PresetMark> >(ModObj, "MarkList");
	
	py::enum_<Coyote::RefreshMode>(ModObj, "RefreshMode")
	EMEMDEF(COYOTE_REFRESH_INVALID)
	EMEMDEF(COYOTE_REFRESH_23_98)
	EMEMDEF(COYOTE_REFRESH_24)
	EMEMDEF(COYOTE_REFRESH_25)
	EMEMDEF(COYOTE_REFRESH_29_97)
	EMEMDEF(COYOTE_REFRESH_30)
	EMEMDEF(COYOTE_REFRESH_50)
	EMEMDEF(COYOTE_REFRESH_59_94)
	EMEMDEF(COYOTE_REFRESH_60)
	EMEMDEF(COYOTE_REFRESH_MAX);
	
	py::enum_<Coyote::ResolutionMode>(ModObj, "ResolutionMode")
	EMEMDEF(COYOTE_RES_INVALID)
	EMEMDEF(COYOTE_RES_1080P)
	EMEMDEF(COYOTE_RES_2160P)
	EMEMDEF(COYOTE_RES_1080I)
	EMEMDEF(COYOTE_RES_MAX);
	
	py::enum_<Coyote::HardwareMode>(ModObj, "HardwareMode")
	EMEMDEF(COYOTE_MODE_INVALID)
	EMEMDEF(COYOTE_MODE_Q3G)
	EMEMDEF(COYOTE_MODE_S12G)
	EMEMDEF(COYOTE_MODE_MAX);
	
	py::enum_<Coyote::StatusCode>(ModObj, "StatusCode")
	EMEMDEF(COYOTE_STATUS_INVALID)
	EMEMDEF(COYOTE_STATUS_OK)
	EMEMDEF(COYOTE_STATUS_FAILED)
	EMEMDEF(COYOTE_STATUS_UNIMPLEMENTED)
	EMEMDEF(COYOTE_STATUS_INTERNALERROR)
	EMEMDEF(COYOTE_STATUS_MISUSED)
	EMEMDEF(COYOTE_STATUS_NETWORKERROR)
	EMEMDEF(COYOTE_STATUS_MAX);
	
	py::class_<Coyote_NetworkInfo>(ModObj, "Coyote_NetworkInfo")
	.def(py::init<>())
	ACLASSBD(Coyote_NetworkInfo, IP)
	ACLASSBD(Coyote_NetworkInfo, Subnet)
	ACLASSBD(Coyote_NetworkInfo, AdapterID);
	
	py::class_<Coyote::NetworkInfo, Coyote::BaseObject, Coyote_NetworkInfo>(ModObj, "NetworkInfo")
	.def(py::init<>());
	
	py::class_<Coyote_HardwareState>(ModObj, "Coyote_HardwareState")
	.def(py::init<>())
	ACLASSBD(Coyote_HardwareState, Resolution)
	ACLASSBD(Coyote_HardwareState, RefreshRate)
	ACLASSBD(Coyote_HardwareState, CurrentMode)
	ACLASSBD(Coyote_HardwareState, SupportsS12G);
	
	py::class_<Coyote::HardwareState, Coyote::BaseObject, Coyote_HardwareState>(ModObj, "HardwareState")
	.def(py::init<>());
	
	
	py::class_<Coyote::Session>(ModObj, "Session")
	.def(py::init<const std::string &>())
	.def("IsMirror",
	[] (Coyote::Session &Obj)
	{
		bool Value{};
		const Coyote::StatusCode Status = Obj.IsMirror(Value);
		
		return std::make_tuple(Status, Value);
	})
	.def("DetectUpdate",
	[] (Coyote::Session &Obj)
	{
		bool Detected{};
		std::string Version;
		const Coyote::StatusCode Status = Obj.DetectUpdate(Detected, &Version);
		
		return std::make_tuple(Status, Detected, Version);
	})
	.def("GetServerVersion",
	[] (Coyote::Session &Obj)
	{
		std::string Version;
		const Coyote::StatusCode Status = Obj.GetServerVersion(Version);
		
		return std::make_tuple(Status, Version);
	})
	.def("GetUnitID",
	[] (Coyote::Session &Obj)
	{
		std::map<std::string, std::string> Vals;
		
		const Coyote::StatusCode Status = Obj.GetUnitID(Vals["UnitID"], Vals["Nickname"]);
		
		return std::make_tuple(Status, Vals);
	})
	.def("GetHardwareState",
	[] (Coyote::Session &Obj)
	{
		Coyote::HardwareState Value;
		
		const Coyote::StatusCode Status = Obj.GetHardwareState(Value);
		
		return std::make_tuple(Status, Value);
	})
	.def("GetTimeCode",
	[] (Coyote::Session &Obj, int32_t PK)
	{
		Coyote::TimeCode Value;
		
		const Coyote::StatusCode Status = Obj.GetTimeCode(Value, PK);
		
		return std::make_tuple(Status, Value);
	})
	.def("GetIP",
	[] (Coyote::Session &Obj, const int32_t AdapterID)
	{
		Coyote::NetworkInfo Value;
		
		const Coyote::StatusCode Status = Obj.GetIP(AdapterID, Value);
		
		return std::make_tuple(Status, Value);
	})
	.def("GetDisks",
	[] (Coyote::Session &Obj)
	{
		std::vector<std::string> Disks;
		
		const Coyote::StatusCode Status = Obj.GetDisks(Disks);
		
		return std::make_tuple(Status, Disks);
	})
	.def("GetPresets",
	[] (Coyote::Session &Obj)
	{
		std::vector<Coyote::Preset> Presets;
		
		const Coyote::StatusCode Status = Obj.GetPresets(Presets);
		
		return std::make_tuple(Status, Presets);
	})
	.def("GetAssets",
	[] (Coyote::Session &Obj)
	{
		std::vector<Coyote::Asset> Assets;
		
		const Coyote::StatusCode Status = Obj.GetAssets(Assets);
		
		return std::make_tuple(Status, Assets);
	})
	.def("GetMirrors",
	[] (Coyote::Session &Obj)
	{
		std::vector<Coyote::Mirror> Mirrors;
		
		const Coyote::StatusCode Status = Obj.GetMirrors(Mirrors);
		
		return std::make_tuple(Status, Mirrors);
	})
	ACLASSF(Session, Take)
	ACLASSF(Session, End)
	ACLASSF(Session, Pause)
	ACLASSF(Session, SeekTo)
	ACLASSF(Session, InstallAsset)
	ACLASSF(Session, DeleteAsset)
	ACLASSF(Session, RenameAsset)
	ACLASSF(Session, ReorderPresets)
	ACLASSF(Session, DeletePreset)
	ACLASSF(Session, CreatePreset)
	ACLASSF(Session, UpdatePreset)
	ACLASSF(Session, LoadPreset)
	ACLASSF(Session, BeginUpdate)
	ACLASSF(Session, DetectUpdate)
	ACLASSF(Session, EjectDisk)
	ACLASSF(Session, RestartService)
	ACLASSF(Session, GetIP)
	ACLASSF(Session, SetIP)
	ACLASSF(Session, InitializeCoyote)
	ACLASSF(Session, SetHardwareMode)
	ACLASSF(Session, SelectPreset)
	ACLASSF(Session, SynchronizerBusy)
	ACLASSF(Session, AddMirror)
	ACLASSF(Session, DeconfigureSync)
	ACLASSF(Session, RebootCoyote)
	ACLASSF(Session, ShutdownCoyote)
	ACLASSF(Session, SoftRebootCoyote)
	ACLASSF(Session, SelectNext)
	ACLASSF(Session, SelectPrev)
	ACLASSF(Session, TakeNext)
	ACLASSF(Session, TakePrev)
	ACLASSF(Session, SetCommandTimeoutSecs)
	ACLASSF(Session, GetCommandTimeoutSecs);
	

	py::class_<Coyote_Output>(ModObj, "Coyote_Output")
	.def(py::init<>())
	ACLASSBD(Coyote_Output, Filename)
	ACLASSBD(Coyote_Output, Hue)
	ACLASSBD(Coyote_Output, Saturation)
	ACLASSBD(Coyote_Output, Contrast)
	ACLASSBD(Coyote_Output, Brightness)
	ACLASSBD(Coyote_Output, MediaId)
	ACLASSBD(Coyote_Output,	FadeOut)
	ACLASSBD(Coyote_Output,	Delay)
	ACLASSBD(Coyote_Output, Active)
	ACLASSBD(Coyote_Output, Audio)
	ACLASSBD(Coyote_Output, AudioChannel1)
	ACLASSBD(Coyote_Output, AudioChannel2)
	ACLASSBD(Coyote_Output, AudioChannel3)
	ACLASSBD(Coyote_Output, AudioChannel4);
	
	py::class_<Coyote::Output, Coyote::BaseObject, Coyote_Output>(ModObj, "Output")
	.def(py::init<>());
	
	py::class_<Coyote_Preset>(ModObj, "Coyote_Preset")
	.def(py::init<>())
	ACLASSBD(Coyote_Preset, PK)
	ACLASSBD(Coyote_Preset, Index)
	ACLASSBD(Coyote_Preset, Loop)
	ACLASSBD(Coyote_Preset, Link)
	ACLASSBD(Coyote_Preset, DisplayLink)
	ACLASSBD(Coyote_Preset, Fade)
	ACLASSBD(Coyote_Preset, LeftVolume)
	ACLASSBD(Coyote_Preset, RightVolume)
	ACLASSBD(Coyote_Preset, ScrubberPosition)
	ACLASSBD(Coyote_Preset, InPosition)
	ACLASSBD(Coyote_Preset, OutPosition)
	ACLASSBD(Coyote_Preset, IsPlaying)
	ACLASSBD(Coyote_Preset, IsPaused)
	ACLASSBD(Coyote_Preset, Selected)
	ACLASSBD(Coyote_Preset, VolumeLinked)
	ACLASSBD(Coyote_Preset, Name)
	ACLASSBD(Coyote_Preset, Layout)
	ACLASSBD(Coyote_Preset, Notes)
	ACLASSBD(Coyote_Preset, Color)
	ACLASSBD(Coyote_Preset, timeCodeUpdate)
	ACLASSBD(Coyote_Preset, tcColor);

	
	py::class_<Coyote::Preset, Coyote::BaseObject, Coyote_Preset>(ModObj, "Preset")
	.def(py::init<>())
	ACLASSD(Preset, Output1)
	ACLASSD(Preset, Output2)
	ACLASSD(Preset, Output3)
	ACLASSD(Preset, Output4)
	ACLASSD(Preset, gotoMarks)
	ACLASSD(Preset, countDowns);
	
	py::class_<Coyote_PresetMark>(ModObj, "Coyote_PresetMark")
	.def(py::init<>())
	ACLASSBD(Coyote_PresetMark, MarkNumber)
	ACLASSBD(Coyote_PresetMark,	MarkName)
	ACLASSBD(Coyote_PresetMark, MarkDisplayTime)
	ACLASSBD(Coyote_PresetMark, MarkTime);

	py::class_<Coyote::PresetMark, Coyote::BaseObject, Coyote_PresetMark>(ModObj, "PresetMark")
	.def(py::init<>());
	
	py::class_<Coyote_TimeCode>(ModObj, "Coyote_TimeCode")
	.def(py::init<>())
	ACLASSBD(Coyote_TimeCode, ScrubBar)
	ACLASSBD(Coyote_TimeCode, Time)
	ACLASSBD(Coyote_TimeCode, TRT)
	ACLASSBD(Coyote_TimeCode, PresetKey)
	ACLASSBD(Coyote_TimeCode, Selected);

	py::class_<Coyote::TimeCode, Coyote::BaseObject, Coyote_TimeCode>(ModObj, "TimeCode")
	.def(py::init<>());

	py::class_<Coyote_Mirror>(ModObj, "Coyote_Mirror")
	.def(py::init<>())
	ACLASSBD(Coyote_Mirror, UnitID)
	ACLASSBD(Coyote_Mirror, IP)
	ACLASSBD(Coyote_Mirror, Busy)
	ACLASSBD(Coyote_Mirror, SupportsS12G);
	
	py::class_<Coyote::Mirror, Coyote::BaseObject, Coyote_Mirror>(ModObj, "Mirror")
	.def(py::init<>());
	
	py::class_<Coyote_MediaState>(ModObj, "Coyote_MediaState")
	.def(py::init<>())
	ACLASSBD(Coyote_MediaState, NumPresets)
	ACLASSBD(Coyote_MediaState, SelectedPreset);
	
	py::class_<Coyote::MediaState, Coyote::BaseObject, Coyote_MediaState>(ModObj, "MediaState")
	.def(py::init<>())
	ACLASSD(MediaState, PlayingPresets)
	ACLASSD(MediaState, PausedPresets)
	ACLASSD(MediaState, Time);
	
	py::class_<Coyote_Asset>(ModObj, "Coyote_Asset")
	.def(py::init<>())
	ACLASSBD(Coyote_Asset, FileName)
	ACLASSBD(Coyote_Asset, NewFileName)
	ACLASSBD(Coyote_Asset, CopyPercentage)
	ACLASSBD(Coyote_Asset, IsReady);
	
	py::class_<Coyote::Asset, Coyote::BaseObject, Coyote_Asset>(ModObj, "Asset")
	.def(py::init<>());

	ModObj.doc() = "Interface for controlling Sonoran Video Systems' Coyote playback servers";	
}
