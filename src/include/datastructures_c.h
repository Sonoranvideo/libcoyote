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

#ifndef __LIBCOYOTE_DATASTRUCTURES_C_H__
#define __LIBCOYOTE_DATASTRUCTURES_C_H__


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "macros.h"
#include "statuscodes.h"


#ifndef __cplusplus
typedef const char *COYOTESTRING;
typedef enum CoyoteHardwareMode COYOTEHARDWAREMODE;
typedef enum CoyoteRefreshRate COYOTEREFRESHMODE;
typedef enum CoyoteResolutionMode COYOTERESOLUTIONMODE;
#else
#include "coyotestring.h"
typedef Coyote::CoyoteString COYOTESTRING;
typedef Coyote::HardwareMode COYOTEHARDWAREMODE;
typedef Coyote::ResolutionMode COYOTERESOLUTIONMODE;
typedef Coyote::RefreshMode COYOTEREFRESHMODE;
typedef Coyote::PresetLayout COYOTEPRESETLAYOUT;
extern "C" {
#endif

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

struct Coyote_Asset
{
	COYOTESTRING FileName; ///The part we care about most
	COYOTESTRING NewFileName; //NewFileName should only be used when renaming/moving assets. Leaving it empty the rest of the time is fine.
	COYOTESTRING Videoencoding_FCC;
	COYOTESTRING Audioencoding_FCC;
	int32_t AudioNumChannels;
	int32_t AudioSampleRate;
	int32_t DurationMs;
	int32_t Size;
	int32_t VideoFrameRate;
	int32_t VideoHeight;
	int32_t VideoWidth;
	uint8_t CopyPercentage; //User shouldn't need to write to this
	bool SupportedVidEncode;
	bool SupportedAudEncode;
	bool IsReady; //or this
};

struct Coyote_Output
{
	COYOTESTRING Filename;
	double FadeOut;
	double Delay;
	int32_t Hue;
	int32_t Saturation;
	int32_t Contrast;
	int32_t Brightness;
	int32_t MediaId;
	int32_t AudioChannel1;
	int32_t AudioChannel2;
	int32_t AudioChannel3;
	int32_t AudioChannel4;
	bool Active;
	bool Audio;
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
	int32_t Link;
	int32_t DisplayLink;
	int32_t Fade;
	int32_t LeftVolume;
	int32_t RightVolume;
	int32_t ScrubberPosition;
	int32_t InPosition;
	int32_t OutPosition;
	bool IsPlaying;
	bool IsPaused;
	bool Selected;
	bool VolumeLinked;
};

struct Coyote_HardwareState
{
	COYOTERESOLUTIONMODE Resolution;
	COYOTEREFRESHMODE RefreshRate;
	COYOTEHARDWAREMODE CurrentMode;
	bool SupportsS12G;
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
