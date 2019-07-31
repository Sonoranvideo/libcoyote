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
#include <string.h>
#include <array>

#include "datastructures_c.h"

namespace Coyote
{
	static_assert(std::is_standard_layout<CoyoteString>::value && alignof(char*) == alignof(CoyoteString), "CoyoteString is no longer a standard layout class due to some modification you made.");
	
	struct BaseObject
	{
		virtual ~BaseObject(void) = default;
	};
	
	
	struct TimeCode : public BaseObject, public Coyote_TimeCode
	{		
		inline operator uint32_t(void) const { return this->Time; }
	};
	
	struct Asset : public BaseObject, public Coyote_Asset
	{				
		inline operator const char*(void) const { return this->FileName; }
		inline operator std::string(void) const { return this->FileName; }
	};
	
	struct Output : public BaseObject, public Coyote_Output
	{
		Output(void) : BaseObject(), Coyote_Output() { }
	};
	
	struct Preset : public BaseObject, public Coyote_Preset
	{
		Output Output1;
		Output Output2;
		Output Output3;
		Output Output4;
		
		inline operator int32_t(void) const { return this->PK; }
		inline Preset(void) : BaseObject(), Coyote_Preset()
		{
			this->Coyote_Preset::Output1 = &this->Output1;
			this->Coyote_Preset::Output2 = &this->Output2;
			this->Coyote_Preset::Output3 = &this->Output3;
			this->Coyote_Preset::Output4 = &this->Output4;
		}
	};
	
	struct HardwareState : public BaseObject, public Coyote_HardwareState
	{
		HardwareState(void) : Coyote_HardwareState() { }
	};
	
	struct NetworkInfo : public BaseObject, public Coyote_NetworkInfo
	{
		NetworkInfo(void) : Coyote_NetworkInfo() { }
	};

	struct MediaState : public BaseObject, public Coyote_MediaState
	{
		std::array<int32_t, COYOTE_MAX_OUTPUTS + 1> PlayingPresets; //One extra for a null terminator
		std::array<int32_t, COYOTE_MAX_OUTPUTS + 1> PausedPresets;
		TimeCode Time;
		
		inline MediaState(void) : BaseObject(), Coyote_MediaState(), PlayingPresets(), PausedPresets()
		{
			this->Coyote_MediaState::PlayingPresets = this->PlayingPresets.data();
			this->Coyote_MediaState::PausedPresets = this->PausedPresets.data();
			this->Coyote_MediaState::Time = &this->Time;
		}
	};
}
#endif //__LIBCOYOTE_DATASTRUCTURES_H__
