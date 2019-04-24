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

#ifndef __LIBCOYOTE_DATASTRUCTURES_H__
#define __LIBCOYOTE_DATASTRUCTURES_H__

#include <stdint.h>
#include <string>


namespace Coyote
{
	struct BaseObject
	{
		virtual ~BaseObject(void) = default;
	};
	
	struct TimeCode : public BaseObject
	{
		double ScrubBar;
		uint32_t Time;
		uint32_t TRT;
		
		inline operator uint32_t(void) const { return this->Time; }
	};
	
	struct Asset : public BaseObject
	{
		std::string FileName; ///The part we care about most
		std::string NewFileName; //NewFileName should only be used when renaming/moving assets. Leaving it empty the rest of the time is fine.
		uint8_t CopyPercentage : 7; //User shouldn't need to write to this
		bool IsReady : 1; //or this
		
		
		inline Asset(const std::string &Path = "") : FileName(Path), NewFileName(), CopyPercentage(100), IsReady(true) {}
		
		inline operator const char*(void) const { return this->FileName.c_str(); }
		inline operator std::string(void) const { return this->FileName; }
	};
	
	struct Output : public BaseObject
	{
		std::string Filename;
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
	
	struct Preset : public BaseObject
	{
		Output Output1;
		Output Output2;
		Output Output3;
		Output Output4;
		std::string Name;
		std::string Layout;
		std::string Notes;
		std::string Color;
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
		
		inline operator int32_t(void) const { return this->PK; }

	};
}
#endif //__LIBCOYOTE_DATASTRUCTURES_H__
