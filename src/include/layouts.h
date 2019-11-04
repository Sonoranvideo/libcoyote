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

#ifndef __LIBCOYOTE_LAYOUTS_H__
#define __LIBCOYOTE_LAYOUTS_H__

#include <string>
#include <map>

#include "statuscodes.h"
#include "datastructures.h"

namespace Coyote
{
	struct LayoutInfo
	{
		std::string TextName;
		Coyote::PresetLayout ID;
		Coyote::Player Players;
		std::map<Coyote::Player, Coyote::SDIOutput> SDIOuts;
	};
	
	Coyote::SDIOutput GetPresetSDIOutputs(const Coyote::Preset &Input);
	Coyote::Player GetPresetPlayers(const Coyote::Preset &Input);
	const LayoutInfo *LookupPresetLayoutByID(const Coyote::PresetLayout LayoutID);
	const LayoutInfo *LookupPresetLayoutByString(const std::string &LayoutName);
	Coyote::SDIOutput GetPlayerSDIOutputs(const Coyote::Preset &Input, const Coyote::Player InPlayer);
}

#endif //__LIBCOYOTE_LAYOUTS_H__
