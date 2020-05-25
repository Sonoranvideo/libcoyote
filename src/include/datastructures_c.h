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

#ifndef __LIBCOYOTE_DATASTRUCTURES_C_H__
#define __LIBCOYOTE_DATASTRUCTURES_C_H__


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "common.h"

#include "macros.h"
#include "statuscodes.h"

#ifndef __cplusplus
typedef const char *COYOTESTRING;
typedef enum CoyoteHardwareMode COYOTEHARDWAREMODE;
typedef enum CoyoteRefreshRate COYOTEREFRESHMODE;
typedef enum CoyoteResolutionMode COYOTERESOLUTIONMODE;
typedef enum CoyoteHDRMode COYOTEHDRMODE;
typedef enum CoyoteEOTFMode COYOTEEOTFMODE;
typedef enum CoyotePlaybackEventType COYOTEPLAYBACKEVENTTYPE;
typedef enum CoyoteAssetState COYOTEASSETSTATE;
typedef enum CoyoteStateEventType COYOTESTATEEVENTTYPE;

#else
#include "coyotestring.h"
typedef Coyote::CoyoteString COYOTESTRING;
typedef Coyote::HardwareMode COYOTEHARDWAREMODE;
typedef Coyote::ResolutionMode COYOTERESOLUTIONMODE;
typedef Coyote::RefreshMode COYOTEREFRESHMODE;
typedef Coyote::PresetLayout COYOTEPRESETLAYOUT;
typedef Coyote::HDRMode COYOTEHDRMODE;
typedef Coyote::EOTFMode COYOTEEOTFMODE;
typedef enum Coyote::PlaybackEventType COYOTEPLAYBACKEVENTTYPE;
typedef enum Coyote::AssetState COYOTEASSETSTATE;
typedef enum Coyote::StateEventType COYOTESTATEEVENTTYPE;

extern "C" {
#endif

typedef void (*PBEventCallback)(const COYOTEPLAYBACKEVENTTYPE EType, const int32_t PK, const int32_t Time, void *UserData);
typedef void (*StateEventCallback)(const COYOTESTATEEVENTTYPE EventType, void *UserData);

struct Coyote_PresetMark
{
	COYOTESTRING MarkNumber;
	COYOTESTRING MarkName;
	COYOTESTRING MarkDisplayTime;
	int32_t MarkTime;
};

struct Coyote_TimeCode
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
};

struct Coyote_Drive
{
	COYOTESTRING DriveLetter;
	int64_t Total, Used, Free;
	bool IsExternal;
};

struct Coyote_Asset
{
	COYOTESTRING FullPath; ///The part we care about most
	COYOTESTRING Checksum;
	int64_t LastModified;
	int64_t TotalSize;
	int64_t CurrentSize;
	COYOTEASSETSTATE Status;
};

struct Coyote_AssetMetadata
{
	COYOTESTRING FullPath;
	COYOTESTRING FPS; //This is not a Coyote::ResolutionMode because it might list a refresh rate that the Coyote doesn't support.
	COYOTESTRING VideoCodec;
	COYOTESTRING AudioCodec;
	int64_t AssetSize;
	int32_t TRT;
	int32_t Width;
	int32_t Height;
	int32_t NumAudioChannels;
	int32_t AudioSampleRate;
	bool SupportedVideoCodec;
	bool SupportedAudioCodec;
};

struct Coyote_Output
{
	COYOTESTRING Filename;
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
};

struct Coyote_Preset
{
	COYOTESTRING Name;
	COYOTEPRESETLAYOUT Layout;
	COYOTESTRING Notes;
	COYOTESTRING Color;
	COYOTESTRING timeCodeUpdate;
	COYOTESTRING tcColor;
	struct Coyote_Output *Output1;
	struct Coyote_Output *Output2;
	struct Coyote_Output *Output3;
	struct Coyote_Output *Output4;
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
};

struct Coyote_HardwareState
{
	COYOTERESOLUTIONMODE Resolution;
	COYOTEREFRESHMODE RefreshRate;
	COYOTEHARDWAREMODE CurrentMode;
	COYOTEHDRMODE HDRMode;
	COYOTEEOTFMODE EOTFSetting;
	bool SupportsS12G;
	bool ConstLumin;
};

struct Coyote_NetworkInfo
{
	COYOTESTRING IP;
	COYOTESTRING Subnet;
	int32_t AdapterID;
};

struct Coyote_MediaState
{
	struct Coyote_TimeCode *TimeCodes;
	int32_t *PlayingPresets;
	int32_t *PausedPresets;
	int32_t NumPresets;
	int32_t SelectedPreset;
};

struct CoyoteSession
{ //Assumed to be exactly the size of a naked pointer
	void *Internal;
};

struct Coyote_PresetArray
{
	void *_Handle; //Used internally
	size_t Length;
	struct Coyote_Preset **Data;
};

struct Coyote_AssetArray
{
	void *_Handle; //Used internally
	size_t Length;
	struct Coyote_Asset **Data;
};

struct Coyote_Mirror
{
	COYOTESTRING UnitID;
	COYOTESTRING IP;
	bool Busy;
	bool SupportsS12G;
	bool IsAlive;
};

struct Coyote_GenlockSettings
{
	COYOTESTRING FormatString;
	int32_t HorzValue;
	int32_t VertValue;
	bool Genlocked;
};

struct Coyote_PresetMarkArray
{
	void *_Handle; //Used internally
	size_t Length;
	struct Coyote_PresetMark **Data;
};

#ifdef __cplusplus
}
#else
void Coyote_AssetArray_Destroy(struct Coyote_AssetArray *Arr);
void Coyote_PresetArray_Destroy(struct Coyote_PresetArray *Arr);

#endif //__cplusplus

#endif //__LIBCOYOTE_DATASTRUCTURES_C_H__
