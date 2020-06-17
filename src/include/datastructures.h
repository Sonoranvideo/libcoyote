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

#ifndef __LIBCOYOTE_DATASTRUCTURES_H__
#define __LIBCOYOTE_DATASTRUCTURES_H__
#include "common.h"

#include <stdint.h>
#include <string>
#include <string.h>
#include <array>
#include <vector>

#include "datastructures_c.h"

namespace Coyote
{
	static_assert(std::is_standard_layout<CoyoteString>::value && alignof(char*) == alignof(CoyoteString), "CoyoteString is no longer a standard layout class due to some modification you made.");

	struct BaseObject
	{
		virtual ~BaseObject(void) = default;
	};
	
	struct PresetMark : public BaseObject, public Coyote_PresetMark
	{
		PresetMark(void) : BaseObject(), Coyote_PresetMark() {}
	};
	
	struct TimeCode : public BaseObject, public Coyote_TimeCode
	{
		inline operator uint32_t(void) const { return this->Time; }
		TimeCode(void) : BaseObject(), Coyote_TimeCode() {}
	};
	
	struct Asset : public BaseObject, public Coyote_Asset
	{				
		inline operator const char*(void) const { return this->FullPath; }
		inline operator std::string(void) const { return this->FullPath; }
		Asset(void) : BaseObject(), Coyote_Asset() {}
	};
	
	struct AssetMetadata : public BaseObject, public Coyote_AssetMetadata
	{
		AssetMetadata(void) : BaseObject(), Coyote_AssetMetadata() {}
	};
	
	struct TabOrdering : public BaseObject, public Coyote_TabOrdering
	{
		TabOrdering(void) : BaseObject(), Coyote_TabOrdering() {}
	};
	
	struct Output : public BaseObject, public Coyote_Output
	{
		Output(void) : BaseObject(), Coyote_Output() {}
	};
	
	struct Preset : public BaseObject, public Coyote_Preset
	{
		Output Output1;
		Output Output2;
		Output Output3;
		Output Output4;
		std::vector<PresetMark> gotoMarks;
		std::vector<PresetMark> countDowns;
		std::unordered_map<int32_t, TabOrdering> TabDisplayOrder;
		
		inline operator int32_t(void) const { return this->PK; }
		inline Preset(void) : BaseObject(), Coyote_Preset()
		{
			this->Coyote_Preset::Output1 = &this->Output1;
			this->Coyote_Preset::Output2 = &this->Output2;
			this->Coyote_Preset::Output3 = &this->Output3;
			this->Coyote_Preset::Output4 = &this->Output4;
		}
	};
	
	struct Drive : public BaseObject, public Coyote_Drive
	{
		Drive(void) : BaseObject(), Coyote_Drive() {}
	};
	
	struct HardwareState : public BaseObject, public Coyote_HardwareState
	{
		HardwareState(void) : Coyote_HardwareState() { }
	};
	
	struct NetworkInfo : public BaseObject, public Coyote_NetworkInfo
	{
		NetworkInfo(void) : Coyote_NetworkInfo() { }
	};
	
	struct Mirror : public BaseObject, public Coyote_Mirror
	{
		Mirror(void) : BaseObject(), Coyote_Mirror() {}
	};
	
	struct GenlockSettings : public BaseObject, public Coyote_GenlockSettings
	{
		GenlockSettings(void) : BaseObject(), Coyote_GenlockSettings() {}
	};
		
	struct MediaState : public BaseObject, public Coyote_MediaState
	{
		std::vector<int32_t> PlayingPresets; //One extra for a null terminator
		std::vector<int32_t> PausedPresets;
		std::vector<TimeCode> TimeCodes;
		
		inline MediaState(void) : BaseObject(), Coyote_MediaState(), PlayingPresets(), PausedPresets()
		{
			this->PlayingPresets.resize(COYOTE_MAX_OUTPUTS + 1);
			this->PausedPresets.resize(COYOTE_MAX_OUTPUTS + 1);
			this->TimeCodes.resize(COYOTE_MAX_OUTPUTS + 1);
			this->Coyote_MediaState::PlayingPresets = this->PlayingPresets.data();
			this->Coyote_MediaState::PausedPresets = this->PausedPresets.data();
			this->Coyote_MediaState::TimeCodes = this->TimeCodes.data();
		}
	};
}
#endif //__LIBCOYOTE_DATASTRUCTURES_H__
