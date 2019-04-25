#ifndef __LIBCOYOTE_DATASTRUCTURES_C_H__
#define __LIBCOYOTE_DATASTRUCTURES_C_H__

#include <stdint.h>
#include <stdbool.h>


#ifndef __cplusplus
typedef const char *COYOTESTRING;
#else
typedef Coyote::CoyoteString COYOTESTRING;
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
	struct Coyote_Output *Output_1;
	struct Coyote_Output *Output_2;
	struct Coyote_Output *Output_3;
	struct Coyote_Output *Output_4;
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


#endif //__LIBCOYOTE_DATASTRUCTURES_C_H__
