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
#include "statuscodes.h"

#ifndef __cplusplus
typedef const char *COYOTESTRING;
typedef enum Coyote_HardwareMode COYOTEHARDWAREMODE;
#else
typedef Coyote::CoyoteString COYOTESTRING;
typedef Coyote::HardwareMode COYOTEHARDWAREMODE;
#endif


struct Coyote_TimeCode
{
	double ScrubBar;
	uint32_t Time;
	uint32_t TRT;
};

struct Coyote_Asset
{
	COYOTESTRING FileName; ///The part we care about most
	COYOTESTRING NewFileName; //NewFileName should only be used when renaming/moving assets. Leaving it empty the rest of the time is fine.
	uint8_t CopyPercentage : 7; //User shouldn't need to write to this
	bool IsReady : 1; //or this
};

struct Coyote_Output
{
	COYOTESTRING Filename;
	int32_t Hue;
	int32_t Saturation;
	int32_t Contrast;
	int32_t Brightness;
	int32_t MediaId;
	double FadeOut;
	double Delay;
	bool Active : 1;
	bool Audio : 1;
};

struct Coyote_Preset
{
	COYOTESTRING Name;
	COYOTESTRING Layout;
	COYOTESTRING Notes;
	COYOTESTRING Color;
	struct Coyote_Output *Output1;
	struct Coyote_Output *Output2;
	struct Coyote_Output *Output3;
	struct Coyote_Output *Output4;
	int32_t PK;
	int32_t Index;
	int32_t Loop;
	int32_t Link;
	int32_t DisplayLink;
	int32_t Fade;
	int32_t LeftVolume;
	int32_t RightVolume;
	bool IsPlaying : 1;
	bool IsPaused : 1;
	bool Selected : 1;
};

struct Coyote_HardwareState
{
	COYOTESTRING Resolution;
	COYOTESTRING RefreshRate;
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
	struct Coyote_TimeCode *Time;
	int32_t *PlayingPresets;
	int32_t *PausedPresets;
	int32_t NumPresets;
	int32_t Selected;
};

#endif //__LIBCOYOTE_DATASTRUCTURES_C_H__
