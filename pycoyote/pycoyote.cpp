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
	.def(py::init<>())
	.def("__str__", &Coyote::CoyoteString::operator std::string)
	.def("__repr__", &Coyote::CoyoteString::operator std::string);
	
	
	py::bind_vector<std::vector<Coyote::Preset> >(ModObj, "PresetList");
	py::bind_vector<std::vector<Coyote::Asset> >(ModObj, "AssetList");
	py::bind_vector<std::vector<std::string> >(ModObj, "StringList");
	
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

	py::class_<Coyote::Session>(ModObj, "Session")
	.def(py::init<const std::string &>())
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
	ACLASSF(Session, IsUpdateDetected)
	ACLASSF(Session, GetDisks)
	ACLASSF(Session, EjectDisk)
	ACLASSF(Session, GetHardwareState)
	ACLASSF(Session, RestartService)
	ACLASSF(Session, GetIP)
	ACLASSF(Session, SetIP)
	ACLASSF(Session, InitializeCoyote)
	ACLASSF(Session, SetHardwareMode)
	ACLASSF(Session, SelectPreset)
	ACLASSF(Session, GetMediaState)
	ACLASSF(Session, GetServerVersion)
	ACLASSF(Session, DetectUpdate)
	ACLASSF(Session, RebootCoyote)
	ACLASSF(Session, ShutdownCoyote)
	ACLASSF(Session, SoftRebootCoyote)
	ACLASSF(Session, TakeNext)
	ACLASSF(Session, TakePrev)
	ACLASSF(Session, GetAssets)
	ACLASSF(Session, GetPresets)
	ACLASSF(Session, GetTimeCode)
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
	ACLASSBD(Coyote_Output, Audio);
	
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
	ACLASSD(Preset, Output4);
	
	py::class_<Coyote_TimeCode>(ModObj, "Coyote_TimeCode")
	.def(py::init<>())
	ACLASSBD(Coyote_TimeCode, ScrubBar)
	ACLASSBD(Coyote_TimeCode, Time)
	ACLASSBD(Coyote_TimeCode, TRT)
	ACLASSBD(Coyote_TimeCode, PresetKey)
	ACLASSBD(Coyote_TimeCode, Selected);

	py::class_<Coyote::TimeCode, Coyote::BaseObject, Coyote_TimeCode>(ModObj, "TimeCode")
	.def(py::init<>());

	py::class_<Coyote_MediaState>(ModObj, "Coyote_MediaState")
	.def(py::init<>())
	ACLASSBD(Coyote_MediaState, NumPresets)
	ACLASSBD(Coyote_MediaState, Selected);
	
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
