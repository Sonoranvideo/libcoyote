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

/**It's important that you use MSGPACK_DEFINE_MAP, not MSGPACK_DEFINE or MSGPACK_DEFINE_ARRAY.
 *Communicator won't understand the data structure otherwise. It looks at items by named text field.**/

#ifndef __LIBCOYOTE_DATASTRUCTURES_H__
#define __LIBCOYOTE_DATASTRUCTURES_H__


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "common.h"

#include "macros.h"
#include "statuscodes.h"


typedef Coyote::HardwareMode HardwareMode;
typedef Coyote::ResolutionMode ResolutionMode;
typedef Coyote::RefreshMode RefreshMode;
typedef Coyote::PresetLayout PresetLayout;
typedef Coyote::HDRMode HDRMode;
typedef Coyote::EOTFMode EOTFMode;

namespace Coyote
{
	typedef void (*PBEventCallback)(const PlaybackEventType EType, const int32_t PK, const int32_t Time, void *UserData);
	typedef void (*StateEventCallback)(const StateEventType EventType, void *UserData);

	struct Object
	{
		virtual ~Object(void) = default;
	};
	
	struct PresetMark : public Object
	{
		std::string MarkNumber;
		std::string MarkName;
		std::string MarkDisplayTime;
		int32_t MarkTime;
		
		MSGPACK_DEFINE_MAP(MarkNumber, MarkName, MarkDisplayTime, MarkTime)
	};
	
	struct TimeCode : public Object
	{
		double ScrubBar;
		int32_t Time;
		int32_t TRT;
		int32_t PresetKey;
		int32_t LeftChannelVolume;
		int32_t RightChannelVolume;
		int32_t Player1LeftVolume;
		int32_t Player1RightVolume;
		int32_t Player2LeftVolume;
		int32_t Player2RightVolume;
		int32_t Player3LeftVolume;
		int32_t Player3RightVolume;
		int32_t Player4LeftVolume;
		int32_t Player4RightVolume;
		bool Selected;
		
		MSGPACK_DEFINE_MAP(
			ScrubBar,
			Time,
			TRT,
			PresetKey,
			LeftChannelVolume,
			RightChannelVolume,
			Player1LeftVolume,
			Player1RightVolume,
			Player2LeftVolume,
			Player2RightVolume,
			Player3LeftVolume,
			Player3RightVolume,
			Player4LeftVolume,
			Player4RightVolume,
			Selected
		)
	};
	
	struct Drive : public Object
	{
		std::string DriveLetter;
		int64_t Total, Used, Free;
		bool IsExternal;
		
		MSGPACK_DEFINE_MAP(DriveLetter, Total, Used, Free, IsExternal)
	};
	
	struct Asset : public Object
	{
		std::string FullPath; ///The part we care about most
		std::string Checksum;
		int64_t LastModified;
		int64_t TotalSize;
		int64_t CurrentSize;
		AssetState Status;
		
		MSGPACK_DEFINE_MAP(FullPath, Checksum, LastModified, TotalSize, CurrentSize, Status)
	};
	
	struct AssetMetadata : public Object
	{
		std::string FullPath;
		std::string FPS; //This is not a Coyote::ResolutionMode because it might list a refresh rate that the Coyote doesn't support.
		std::string VideoCodec;
		std::string AudioCodec;
		int64_t AssetSize;
		int32_t TRT;
		int32_t Width;
		int32_t Height;
		int32_t NumAudioChannels;
		int32_t AudioSampleRate;
		bool SupportedVideoCodec;
		bool SupportedAudioCodec;
		
		MSGPACK_DEFINE_MAP(
			FullPath,
			FPS,
			VideoCodec,
			AudioCodec,
			AssetSize,
			TRT,
			Width,
			Height,
			NumAudioChannels,
			AudioSampleRate,
			SupportedVideoCodec,
			SupportedAudioCodec
		)
	};
	
	struct TabOrdering : public Object
	{
		int32_t TabID;
		int32_t Index;
		
		MSGPACK_DEFINE_MAP(TabID, Index)
	};
	
	struct Output : public Object
	{
		std::string Filename;
		double 	FadeOut;
		double 	Delay;
		int32_t	Hue;
		int32_t	Saturation;
		int32_t	Contrast;
		int32_t	Brightness;
		int32_t	MediaId;
		int32_t	AudioChannel1;
		int32_t	AudioChannel2;
		int32_t	AudioChannel3;
		int32_t	AudioChannel4;
		int32_t	OriginalHeight;
		int32_t	OriginalWidth;
		int32_t	CustomDestX;
		int32_t	CustomDestY;
		int32_t	CustHeight;
		int32_t	CustWidth;
		int32_t	HorizontalCrop;
		int32_t	VerticalCrop;
	
		bool	JustifyTop;
		bool	JustifyBottom;
		bool	JustifyRight;
		bool	JustifyLeft;
		bool	CenterVideo;
		bool	NativeSize;
		bool	LetterPillarBox;
		bool	TempFlag;
		bool	Anamorphic;
		bool	MultiviewAudio;
		bool	EnableTimeCode;
		bool	Active;
		bool	Audio;
		
		MSGPACK_DEFINE_MAP(
			Filename,
			FadeOut,
			Delay,
			Hue,
			Saturation,
			Contrast,
			Brightness,
			MediaId,
			AudioChannel1,
			AudioChannel2,
			AudioChannel3,
			AudioChannel4,
			OriginalHeight,
			OriginalWidth,
			CustomDestX,
			CustomDestY,
			CustHeight,
			CustWidth,
			HorizontalCrop,
			VerticalCrop,
			JustifyTop,
			JustifyBottom,
			JustifyRight,
			JustifyLeft,
			CenterVideo,
			NativeSize,
			LetterPillarBox,
			TempFlag,
			Anamorphic,
			MultiviewAudio,
			EnableTimeCode,
			Active,
			Audio
		)
	};
	
	struct ExternalAsset : public Object
	{
		std::string Filename;
		std::string FullPath;
		int64_t FileSize;
		bool IsDirectory;
		
		MSGPACK_DEFINE_MAP(Filename, FullPath, FileSize, IsDirectory)
	};
	
	struct Preset : public Object
	{
		std::string Name;
		PresetLayout Layout;
		std::string Notes;
		std::string Color;
		std::string timeCodeUpdate;
		std::string tcColor;
		Output Output1;
		Output Output2;
		Output Output3;
		Output Output4;
		std::vector<PresetMark> gotoMarks;
		std::vector<PresetMark> countDowns;
		std::unordered_map<int32_t, TabOrdering> TabDisplayOrder;
		int32_t PK;
		int32_t TRT;
		int32_t Index;
		int32_t Loop;
		int32_t TotalLoop;
		int32_t Link;
		int32_t DisplayLink;
		int32_t Fade;
		int32_t LeftVolume;
		int32_t RightVolume;
		int32_t ScrubberPosition;
		int32_t InPosition;
		int32_t OutPosition;
		int32_t DisplayOrderIndex;
		int32_t Dissolve;
		bool IsPlaying;
		bool IsPaused;
		bool Selected;
		bool VolumeLinked;
		bool FreezeAtEnd;
		
		MSGPACK_DEFINE_MAP(
			Name,
			Layout,
			Notes,
			Color,
			timeCodeUpdate,
			tcColor,
			Output1,
			Output2,
			Output3,
			Output4,		
			gotoMarks,
			countDowns,
			TabDisplayOrder,
			PK,
			TRT,
			Index,
			Loop,
			TotalLoop,
			Link,
			DisplayLink,
			Fade,
			LeftVolume,
			RightVolume,
			ScrubberPosition,
			InPosition,
			OutPosition,
			DisplayOrderIndex,
			Dissolve,
			IsPlaying,
			IsPaused,
			Selected,
			VolumeLinked,
			FreezeAtEnd
		)
	};
	
	struct HardwareState : public Object
	{
		ResolutionMode Resolution;
		RefreshMode RefreshRate;
		HardwareMode CurrentMode;
		enum HDRMode HDRMode;
		EOTFMode EOTFSetting;
		bool SupportsS12G;
		bool ConstLumin;
		
		MSGPACK_DEFINE_MAP(
			Resolution,
			RefreshRate,
			CurrentMode,
			HDRMode,
			EOTFSetting,
			SupportsS12G,
			ConstLumin
		)
	};
	
	struct NetworkInfo : public Object
	{
		std::string IP;
		std::string Subnet;
		int32_t AdapterID;
		
		MSGPACK_DEFINE_MAP(IP, Subnet, AdapterID)
	};
	
	struct MediaState : public Object
	{
		std::vector<TimeCode> TimeCodes;
		std::vector<int32_t> PlayingPresets;
		std::vector<int32_t> PausedPresets;
		
		int32_t NumPresets;
		int32_t SelectedPreset;

		inline MediaState(void) : Object()
		{
			this->PlayingPresets.resize(COYOTE_MAX_OUTPUTS);
			this->PausedPresets.resize(COYOTE_MAX_OUTPUTS);
			this->TimeCodes.resize(COYOTE_MAX_OUTPUTS);
		}
		
		MSGPACK_DEFINE_MAP(TimeCodes, PlayingPresets, PausedPresets, NumPresets, SelectedPreset)
	};
	
	struct Mirror : public Object
	{
		std::string UnitID;
		std::string IP;
		bool Busy;
		bool SupportsS12G;
		bool IsAlive;
		
		MSGPACK_DEFINE_MAP(UnitID, IP, Busy, SupportsS12G, IsAlive)
	};
	
	struct GenlockSettings : public Object
	{
		std::string FormatString;
		int32_t HorzValue;
		int32_t VertValue;
		bool Genlocked;
		
		MSGPACK_DEFINE_MAP(FormatString, HorzValue, VertValue, Genlocked)
	};

}
#endif //__LIBCOYOTE_DATASTRUCTURES_H__
