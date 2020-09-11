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

#ifndef __LIBCOYOTE_LAYOUTS_H__
#define __LIBCOYOTE_LAYOUTS_H__

#include <string>
#include <map>
#include "common.h"

#include "statuscodes.h"
#include "datastructures.h"

namespace Coyote
{
	struct Layout2D
	{
		uint32_t X, Y;
		
		bool operator==(const Layout2D &Other) const
		{
			return Other.X == this->X && Other.Y == this->Y;
		}
		
		bool operator<(const Layout2D &RHS) const
		{ //Only used for sorting into maps.
			return this->X < RHS.X && this->Y < RHS.Y;
		}
		
		Layout2D(const uint32_t X = 0, const uint32_t Y = 0) : X(X), Y(Y) {}
	};
		
	struct LayoutInfo
	{
		std::string TextName;
		Coyote::PresetLayout ID;
		Coyote::Player Players;
		std::unordered_map<Coyote::Player, Coyote::SDIOutput> SDIOuts;
		std::unordered_map<Coyote::SDIOutput, Coyote::Layout2D> GridOrientation;
		std::unordered_map<Coyote::Player, Coyote::Layout2D> PlayerResolutions;
	};
	
	EXPFUNC Coyote::SDIOutput GetPresetSDIOutputs(const Coyote::Preset &Input);
	EXPFUNC Coyote::Player GetPresetPlayers(const Coyote::Preset &Input);
	EXPFUNC const LayoutInfo *LookupPresetLayoutByID(const Coyote::PresetLayout LayoutID);
	EXPFUNC const LayoutInfo *LookupPresetLayoutByString(const std::string &LayoutName);
	EXPFUNC Coyote::SDIOutput GetPlayerSDIOutputs(const Coyote::Preset &Input, const Coyote::Player InPlayer);
}

#endif //__LIBCOYOTE_LAYOUTS_H__
