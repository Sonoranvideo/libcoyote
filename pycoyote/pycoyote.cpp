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
#include <Python.h>
#include "../src/include/common.h"
#include "../src/include/datastructures.h"
#include "../src/include/session.h"
#include "../src/include/layouts.h"
#include <stddef.h>
#include <stdint.h>
#include "pybind11/include/pybind11/pybind11.h"
#include "pybind11/include/pybind11/stl.h"
#include "pybind11/include/pybind11/stl_bind.h"

extern EXPFUNC const std::map<Coyote::RefreshMode, std::string> RefreshMap;
extern EXPFUNC const std::map<Coyote::ResolutionMode, std::string> ResolutionMap;

namespace py = pybind11;

#define ACLASSF(a, b) .def(#b, &Coyote::a::b, py::call_guard<py::gil_scoped_release>())
#define ACLASSD(a, b) .def_readwrite(#b, &Coyote::a::b)
#define ACLASSBD(a, b) .def_readwrite(#b, &a::b)

#define EMEMDEF(a) .value(#a, Coyote::a)
static void PBEventFunc(const Coyote::PlaybackEventType EType, const int32_t PK, const int32_t Time, void *const Pass_);
	
PYBIND11_MODULE(pycoyote, ModObj)
{
	ModObj.def("LookupPresetLayoutByID", Coyote::LookupPresetLayoutByID);
	ModObj.def("LookupPresetLayoutByString", Coyote::LookupPresetLayoutByString);
	ModObj.def("GetPresetPlayers", Coyote::GetPresetPlayers);
	ModObj.def("GetPresetSDIOutputs", Coyote::GetPresetSDIOutputs);
	ModObj.def("GetPlayerSDIOutputs", Coyote::GetPlayerSDIOutputs);
	
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
	EMEMDEF(COYOTE_REFRESH_MAX)
	.export_values();
	
	py::enum_<Coyote::ResolutionMode>(ModObj, "ResolutionMode")
	EMEMDEF(COYOTE_RES_INVALID)
	EMEMDEF(COYOTE_RES_1080P)
	EMEMDEF(COYOTE_RES_2160P)
	EMEMDEF(COYOTE_RES_1080I)
	EMEMDEF(COYOTE_RES_MAX)
	.export_values();
	
	py::enum_<Coyote::HDRMode>(ModObj, "HDRMode")
	EMEMDEF(COYOTE_HDR_DISABLED)
	EMEMDEF(COYOTE_HDR_BT2020)
	EMEMDEF(COYOTE_HDR_DCI_P3)
	EMEMDEF(COYOTE_HDR_DOLBY)
	EMEMDEF(COYOTE_HDR_MAX)
	.export_values();
	
	py::enum_<Coyote::EOTFMode>(ModObj, "EOTFMode")
	EMEMDEF(COYOTE_EOTF_NORMAL)
	EMEMDEF(COYOTE_EOTF_HLG)
	EMEMDEF(COYOTE_EOTF_PQ)
	EMEMDEF(COYOTE_EOTF_UNSPECIFIED)
	EMEMDEF(COYOTE_EOTF_MAX)
	.export_values();
	
	py::enum_<Coyote::PlaybackEventType>(ModObj, "PlaybackEventType")
	EMEMDEF(COYOTE_PBEVENT_END)
	EMEMDEF(COYOTE_PBEVENT_TAKE)
	EMEMDEF(COYOTE_PBEVENT_PAUSE)
	EMEMDEF(COYOTE_PBEVENT_UNPAUSE)
	EMEMDEF(COYOTE_PBEVENT_SEEK)
	.export_values();
		
	
	py::enum_<Coyote::HardwareMode>(ModObj, "HardwareMode")
	EMEMDEF(COYOTE_MODE_INVALID)
	EMEMDEF(COYOTE_MODE_Q3G)
	EMEMDEF(COYOTE_MODE_S12G)
	EMEMDEF(COYOTE_MODE_MAX)
	.export_values();
	
	py::enum_<Coyote::StatusCode>(ModObj, "StatusCode")
	EMEMDEF(COYOTE_STATUS_INVALID)
	EMEMDEF(COYOTE_STATUS_OK)
	EMEMDEF(COYOTE_STATUS_FAILED)
	EMEMDEF(COYOTE_STATUS_UNIMPLEMENTED)
	EMEMDEF(COYOTE_STATUS_INTERNALERROR)
	EMEMDEF(COYOTE_STATUS_MISUSED)
	EMEMDEF(COYOTE_STATUS_NETWORKERROR)
	EMEMDEF(COYOTE_STATUS_MAX)
	.export_values();
	
	py::enum_<Coyote::UnitRole>(ModObj, "UnitRole")
	EMEMDEF(COYOTE_ROLE_INVALID) 
	EMEMDEF(COYOTE_ROLE_SINGLE)
	EMEMDEF(COYOTE_ROLE_PRIMARY)
	EMEMDEF(COYOTE_ROLE_MIRROR)
	EMEMDEF(COYOTE_ROLE_MAX)
	.export_values();

	
	py::enum_<Coyote::PresetLayout>(ModObj, "PresetLayout")
	EMEMDEF(COYOTE_PSLAYOUT_INVALID)
	EMEMDEF(COYOTE_PSLAYOUT_A)
	EMEMDEF(COYOTE_PSLAYOUT_B)
	EMEMDEF(COYOTE_PSLAYOUT_C1)
	EMEMDEF(COYOTE_PSLAYOUT_C2)
	EMEMDEF(COYOTE_PSLAYOUT_C3)
	EMEMDEF(COYOTE_PSLAYOUT_F)
	EMEMDEF(COYOTE_PSLAYOUT_E)
	EMEMDEF(COYOTE_PSLAYOUT_D1)
	EMEMDEF(COYOTE_PSLAYOUT_D2)
	EMEMDEF(COYOTE_PSLAYOUT_MAX)
	.export_values();

	
	py::enum_<Coyote::SDIOutput>(ModObj, "SDIOutput", py::arithmetic())
	EMEMDEF(COYOTE_SDI_INVALID)
	EMEMDEF(COYOTE_SDI_1)
	EMEMDEF(COYOTE_SDI_2)
	EMEMDEF(COYOTE_SDI_3)
	EMEMDEF(COYOTE_SDI_4)
	EMEMDEF(COYOTE_SDI_MAXVALUE)
	.export_values();

	
	py::enum_<Coyote::Player>(ModObj, "Player", py::arithmetic())
	EMEMDEF(COYOTE_PLAYER_INVALID)
	EMEMDEF(COYOTE_PLAYER_1)
	EMEMDEF(COYOTE_PLAYER_2)
	EMEMDEF(COYOTE_PLAYER_3)
	EMEMDEF(COYOTE_PLAYER_4)
	EMEMDEF(COYOTE_PLAYER_MAXVALUE)
	.export_values();

	
	py::class_<Coyote_NetworkInfo>(ModObj, "Coyote_NetworkInfo")
	.def(py::init<>())
	ACLASSBD(Coyote_NetworkInfo, IP)
	ACLASSBD(Coyote_NetworkInfo, Subnet)
	ACLASSBD(Coyote_NetworkInfo, AdapterID);
	
	py::class_<Coyote::NetworkInfo, Coyote::BaseObject, Coyote_NetworkInfo>(ModObj, "NetworkInfo")
	.def("__repr__", [] (Coyote::NetworkInfo &Obj) { return std::string{"<NetworkInfo, IP "} + Obj.IP.GetStdString() + ", subnet " + Obj.Subnet.GetStdString() + ">"; })
	.def(py::init<>());
	
	py::class_<Coyote_HardwareState>(ModObj, "Coyote_HardwareState")
	.def(py::init<>())
	ACLASSBD(Coyote_HardwareState, Resolution)
	ACLASSBD(Coyote_HardwareState, RefreshRate)
	ACLASSBD(Coyote_HardwareState, CurrentMode)
	ACLASSBD(Coyote_HardwareState, HDRMode)
	ACLASSBD(Coyote_HardwareState, EOTFSetting)
	ACLASSBD(Coyote_HardwareState, SupportsS12G)
	ACLASSBD(Coyote_HardwareState, ConstLumin);
	
	py::class_<Coyote::HardwareState, Coyote::BaseObject, Coyote_HardwareState>(ModObj, "HardwareState")
	.def(py::init<>())
	.def("__repr__", [] (Coyote::HardwareState &Obj) { return std::string{"<HardwareState of "} + ResolutionMap.at(Obj.Resolution) + "@" + RefreshMap.at(Obj.RefreshRate) + ">"; });
	
	
	py::class_<Coyote::Session>(ModObj, "Session")
	.def(py::init<const std::string &>())
	.def("SynchronizerBusy",
	[] (Coyote::Session &Obj)
	{
		bool Value{};
		const Coyote::StatusCode Status = Obj.SynchronizerBusy(Value);
		
		return std::make_tuple(Status, Value);
	}, py::call_guard<py::gil_scoped_release>())
	.def("ReadLog",
	[] (Coyote::Session &Obj, const std::string SpokeName, const int Year, const int Month, const int Day)
	{
		std::string LogText;
		const Coyote::StatusCode Status = Obj.ReadLog(SpokeName, Year, Month, Day, LogText);
		
		return std::make_tuple(Status, LogText);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetLogsZip",
	[] (Coyote::Session &Obj)
	{
		std::vector<uint8_t> Bytes;
		
		const Coyote::StatusCode Status = Obj.GetLogsZip(Bytes);
		
		return std::make_tuple(Status, py::bytes((const char*)Bytes.data(), Bytes.size()));
	}, py::call_guard<py::gil_scoped_release>())
	.def("IsMirror",
	[] (Coyote::Session &Obj)
	{
		bool Value{};
		const Coyote::StatusCode Status = Obj.IsMirror(Value);
		
		return std::make_tuple(Status, Value);
	}, py::call_guard<py::gil_scoped_release>())
	.def("DetectUpdate",
	[] (Coyote::Session &Obj)
	{
		bool Detected{};
		std::string Version;
		const Coyote::StatusCode Status = Obj.DetectUpdate(Detected, &Version);
		
		return std::make_tuple(Status, Detected, Version);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetMediaState",
	[] (Coyote::Session &Obj)
	{
		Coyote::MediaState State{};
		
		const Coyote::StatusCode Status = Obj.GetMediaState(State);
		
		int MaxPlayingIndex = 0;
		int MaxPausedIndex = 0;
		int MaxTCIndex = 0;
		
		while (State.PlayingPresets[MaxPlayingIndex]) ++MaxPlayingIndex;
		while (State.PausedPresets[MaxPausedIndex]) ++MaxPausedIndex;
		while (State.TimeCodes[MaxTCIndex].PresetKey) ++MaxTCIndex;
		
		State.PausedPresets.resize(MaxPausedIndex);
		State.PlayingPresets.resize(MaxPlayingIndex);
		State.TimeCodes.resize(MaxTCIndex);
		
		return std::make_tuple(Status, State);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetServerVersion",
	[] (Coyote::Session &Obj)
	{
		std::string Version;
		const Coyote::StatusCode Status = Obj.GetServerVersion(Version);
		
		return std::make_tuple(Status, Version);
	})
	.def("SetPlaybackEventCallback", 
	[] (Coyote::Session &Obj, py::function Func, py::object PyUserData)
	{
		static std::pair<py::function, py::object> Pass;
		
		Func.inc_ref();
		Func.inc_ref();
		PyUserData.inc_ref();
		PyUserData.inc_ref();
		
		Pass =	{
					std::move(Func),
					std::move(PyUserData)
				};
		
		Obj.SetPlaybackEventCallback(PBEventFunc, &Pass);
	})
	.def("GetHardwareState",
	[] (Coyote::Session &Obj)
	{
		Coyote::HardwareState Value{};
		
		const Coyote::StatusCode Status = Obj.GetHardwareState(Value);
		
		return std::make_tuple(Status, Value);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetCurrentRole",
	[] (Coyote::Session &Obj)
	{
		Coyote::UnitRole Value{};
		
		const Coyote::StatusCode Status = Obj.GetCurrentRole(Value);
		
		return std::make_tuple(Status, Value);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetUnitID",
	[] (Coyote::Session &Obj)
	{
		std::string UnitID, Nickname;
		
		const Coyote::StatusCode Status = Obj.GetUnitID(UnitID, Nickname);
		
		return std::make_tuple(Status, UnitID, Nickname);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetTimeCode",
	[] (Coyote::Session &Obj, int32_t PK)
	{
		Coyote::TimeCode Value{};
		
		const Coyote::StatusCode Status = Obj.GetTimeCode(Value, PK);
		
		return std::make_tuple(Status, Value);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetIP",
	[] (Coyote::Session &Obj, const int32_t AdapterID)
	{
		Coyote::NetworkInfo Value{};
		
		const Coyote::StatusCode Status = Obj.GetIP(AdapterID, Value);
		
		return std::make_tuple(Status, Value);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetDisks",
	[] (Coyote::Session &Obj)
	{
		std::vector<std::string> Disks;
		
		const Coyote::StatusCode Status = Obj.GetDisks(Disks);
		
		return std::make_tuple(Status, Disks);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetPresets",
	[] (Coyote::Session &Obj)
	{
		std::vector<Coyote::Preset> Presets;
		
		const Coyote::StatusCode Status = Obj.GetPresets(Presets);
		
		return std::make_tuple(Status, Presets);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetAssets",
	[] (Coyote::Session &Obj)
	{
		std::vector<Coyote::Asset> Assets;
		
		const Coyote::StatusCode Status = Obj.GetAssets(Assets);
		
		return std::make_tuple(Status, Assets);
	}, py::call_guard<py::gil_scoped_release>())
	.def("GetMirrors",
	[] (Coyote::Session &Obj)
	{
		std::vector<Coyote::Mirror> Mirrors;
		
		const Coyote::StatusCode Status = Obj.GetMirrors(Mirrors);
		
		return std::make_tuple(Status, Mirrors);
	}, py::call_guard<py::gil_scoped_release>())
	ACLASSF(Session, Take)
	ACLASSF(Session, End)
	ACLASSF(Session, Pause)
	ACLASSF(Session, SetPause)
	ACLASSF(Session, UnsetPause)
	ACLASSF(Session, SetPausedState)
	ACLASSF(Session, SeekTo)
	ACLASSF(Session, InstallAsset)
	ACLASSF(Session, DeleteAsset)
	ACLASSF(Session, RenameAsset)
	ACLASSF(Session, ReorderPresets)
	ACLASSF(Session, DeletePreset)
	ACLASSF(Session, CreatePreset)
	ACLASSF(Session, UpdatePreset)
	ACLASSF(Session, BeginUpdate)
	ACLASSF(Session, DetectUpdate)
	ACLASSF(Session, EjectDisk)
	ACLASSF(Session, SetIP)
	ACLASSF(Session, SetHardwareMode)
	ACLASSF(Session, SelectPreset)
	ACLASSF(Session, AddMirror)
	ACLASSF(Session, DeconfigureSync)
	ACLASSF(Session, RebootCoyote)
	ACLASSF(Session, ShutdownCoyote)
	ACLASSF(Session, SoftRebootCoyote)
	ACLASSF(Session, SelectNext)
	ACLASSF(Session, SelectPrev)
	ACLASSF(Session, DeleteGoto)
	ACLASSF(Session, DeleteCountdown)
	ACLASSF(Session, RenameGoto)
	ACLASSF(Session, RenameCountdown)
	ACLASSF(Session, CreateGoto)
	ACLASSF(Session, CreateCountdown)
	ACLASSF(Session, TakeNext)
	ACLASSF(Session, TakePrev)
	ACLASSF(Session, ExportLogsZip)
	ACLASSF(Session, SetUnitNickname)
	ACLASSF(Session, SetCommandTimeoutSecs)
	ACLASSF(Session, GetCommandTimeoutSecs)
	ACLASSF(Session, HasConnectionError);
	

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
	ACLASSBD(Coyote_Output, AudioChannel4)
	ACLASSBD(Coyote_Output, EnableTimeCode)
	ACLASSBD(Coyote_Output, OriginalHeight)
	ACLASSBD(Coyote_Output, OriginalWidth)
	ACLASSBD(Coyote_Output, CustomDestX)
	ACLASSBD(Coyote_Output, CustomDestY)
	ACLASSBD(Coyote_Output, CustHeight)
	ACLASSBD(Coyote_Output, CustWidth)
	ACLASSBD(Coyote_Output, JustifyTop)
	ACLASSBD(Coyote_Output, JustifyBottom)
	ACLASSBD(Coyote_Output, JustifyRight)
	ACLASSBD(Coyote_Output, JustifyLeft)
	ACLASSBD(Coyote_Output, CenterVideo)
	ACLASSBD(Coyote_Output, NativeSize)
	ACLASSBD(Coyote_Output, LetterPillarBox)
	ACLASSBD(Coyote_Output, TempFlag)
	ACLASSBD(Coyote_Output, Anamorphic)
	ACLASSBD(Coyote_Output, MultiviewAudio)
	ACLASSBD(Coyote_Output, HorizontalCrop)
	ACLASSBD(Coyote_Output, VerticalCrop);
	
	py::class_<Coyote::Output, Coyote::BaseObject, Coyote_Output>(ModObj, "Output")
	.def(py::init<>());
	
	py::class_<Coyote_Preset>(ModObj, "Coyote_Preset")
	.def(py::init<>())
	ACLASSBD(Coyote_Preset, PK)
	ACLASSBD(Coyote_Preset, TRT)
	ACLASSBD(Coyote_Preset, Index)
	ACLASSBD(Coyote_Preset, Loop)
	ACLASSBD(Coyote_Preset, TotalLoop)
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
	ACLASSBD(Coyote_Preset, tcColor)
	ACLASSBD(Coyote_Preset, FreezeAtEnd)
	ACLASSBD(Coyote_Preset, DisplayOrderIndex)
	ACLASSBD(Coyote_Preset, Dissolve);

	
	py::class_<Coyote::Preset, Coyote::BaseObject, Coyote_Preset>(ModObj, "Preset")
	.def(py::init<>())
	.def("__repr__", [] (Coyote::Preset &Obj) { return std::string{"<Preset \""} + Obj.Name.GetStdString() + "\", PK " + std::to_string(Obj.PK) + ", TRT " + std::to_string(Obj.TRT) + ">"; })
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
	.def(py::init<>())
	.def("__repr__", [] (Coyote::PresetMark &Obj) { return std::string{"<PresetMark \""} + Obj.MarkName.GetStdString() + "\", time " + std::to_string(Obj.MarkTime) + ">"; });
	
	py::class_<Coyote_TimeCode>(ModObj, "Coyote_TimeCode")
	.def(py::init<>())
	ACLASSBD(Coyote_TimeCode, ScrubBar)
	ACLASSBD(Coyote_TimeCode, Time)
	ACLASSBD(Coyote_TimeCode, TRT)
	ACLASSBD(Coyote_TimeCode, PresetKey)
	ACLASSBD(Coyote_TimeCode, LeftChannelVolume)
	ACLASSBD(Coyote_TimeCode, RightChannelVolume)
	ACLASSBD(Coyote_TimeCode, Player1LeftVolume)
	ACLASSBD(Coyote_TimeCode, Player1RightVolume)
	ACLASSBD(Coyote_TimeCode, Player2LeftVolume)
	ACLASSBD(Coyote_TimeCode, Player2RightVolume)
	ACLASSBD(Coyote_TimeCode, Player3LeftVolume)
	ACLASSBD(Coyote_TimeCode, Player3RightVolume)
	ACLASSBD(Coyote_TimeCode, Player4LeftVolume)
	ACLASSBD(Coyote_TimeCode, Player4RightVolume)
	ACLASSBD(Coyote_TimeCode, Selected);

	py::class_<Coyote::TimeCode, Coyote::BaseObject, Coyote_TimeCode>(ModObj, "TimeCode")
	.def(py::init<>())
	.def("__repr__", [] (Coyote::TimeCode &Obj) { return std::string{"<TimeCode for PK "} + std::to_string(Obj.PresetKey) + " with time " + std::to_string(Obj.Time) + ">"; });

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
	ACLASSD(MediaState, TimeCodes);
	
	py::class_<Coyote::LayoutInfo>(ModObj, "LayoutInfo")
	.def(py::init<>())
	.def("__repr__", [] (Coyote::LayoutInfo &Obj) { return std::string{"<LayoutInfo for preset layout "} + Obj.TextName + ">"; })
	ACLASSD(LayoutInfo, TextName)
	ACLASSD(LayoutInfo, ID)
	ACLASSD(LayoutInfo, Players)
	ACLASSD(LayoutInfo, SDIOuts);
	
	py::class_<Coyote_Asset>(ModObj, "Coyote_Asset")
	.def(py::init<>())
	ACLASSBD(Coyote_Asset, FileName)
	ACLASSBD(Coyote_Asset, NewFileName)
	ACLASSBD(Coyote_Asset, CopyPercentage)
	ACLASSBD(Coyote_Asset, IsReady)
	ACLASSBD(Coyote_Asset, AudioNumChannels)
	ACLASSBD(Coyote_Asset, AudioSampleRate)
	ACLASSBD(Coyote_Asset, DurationMs)
	ACLASSBD(Coyote_Asset, Size)
	ACLASSBD(Coyote_Asset, VideoFrameRate)
	ACLASSBD(Coyote_Asset, VideoHeight)
	ACLASSBD(Coyote_Asset, VideoWidth)
	ACLASSBD(Coyote_Asset, Videoencoding_FCC)
	ACLASSBD(Coyote_Asset, Audioencoding_FCC)
	ACLASSBD(Coyote_Asset, SupportedVidEncode)
	ACLASSBD(Coyote_Asset, SupportedAudEncode);
	
	py::class_<Coyote::Asset, Coyote::BaseObject, Coyote_Asset>(ModObj, "Asset")
	.def(py::init<>())
	.def("__repr__", [] (Coyote::Asset &Obj) { return std::string{"<Asset \""} + Obj.FileName.GetStdString() + "\">"; });

	ModObj.doc() = "Interface for controlling Sonoran Video Systems' Coyote playback servers";	
}

static void PBEventFunc(const Coyote::PlaybackEventType EType, const int32_t PK, const int32_t Time, void *const Pass_)
{
	py::gil_scoped_acquire GILLock;
	
	std::pair<py::function, py::object> *const Pass = (decltype(Pass))Pass_;

	if (!Pass || !Pass->first || !PyCallable_Check(Pass->first.ptr()))
	{
		LDEBUG_MSG("No pass");
		return;
	}
	
	Pass->first(EType, PK, Time, Pass->second);
}
