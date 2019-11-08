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

///This file should have never needed to exist in the first place.

#include "common.h"
#include "include/layouts.h"
#include "include/statuscodes.h"
#include "include/datastructures.h"

using namespace Coyote;

static const std::map<Coyote::PresetLayout, Coyote::LayoutInfo> Layouts
{
	{
		COYOTE_PSLAYOUT_A,
		{
			"A", COYOTE_PSLAYOUT_A, (Player)(COYOTE_PLAYER_1 | COYOTE_PLAYER_2 | COYOTE_PLAYER_3 | COYOTE_PLAYER_4 ),
			{
				{ COYOTE_PLAYER_1, COYOTE_SDI_1 }, //The SDI mappings only apply in 3G mode, as 12G will fill the whole screen and pump a downscaled copy of all 4 down each SDI lane.
				{ COYOTE_PLAYER_2, COYOTE_SDI_2 },
				{ COYOTE_PLAYER_3, COYOTE_SDI_3 },
				{ COYOTE_PLAYER_4, COYOTE_SDI_4 }
			}
		}
	},
	{
		COYOTE_PSLAYOUT_B,
		{
			"B", COYOTE_PSLAYOUT_B, (Player)(COYOTE_PLAYER_1 | COYOTE_PLAYER_3),
			{
				{ COYOTE_PLAYER_1, (SDIOutput)(COYOTE_SDI_1 | COYOTE_SDI_2) },
				{ COYOTE_PLAYER_3, (SDIOutput)(COYOTE_SDI_3 | COYOTE_SDI_4) },
				
			}
		}
	},
	{
		COYOTE_PSLAYOUT_C1,
		{
			"C1", COYOTE_PSLAYOUT_C1, (Player)(COYOTE_PLAYER_1 | COYOTE_PLAYER_3 | COYOTE_PLAYER_4),
			{
				{ COYOTE_PLAYER_1, (SDIOutput)(COYOTE_SDI_1 | COYOTE_SDI_2) },
				{ COYOTE_PLAYER_3, COYOTE_SDI_3 },
				{ COYOTE_PLAYER_4, COYOTE_SDI_4 },
			}
		}
	},
	{
		COYOTE_PSLAYOUT_C2,
		{
			"C2", COYOTE_PSLAYOUT_C2, (Player)(COYOTE_PLAYER_1 | COYOTE_PLAYER_2 | COYOTE_PLAYER_4),
			{
				{ COYOTE_PLAYER_1, COYOTE_SDI_1 },
				{ COYOTE_PLAYER_2, (SDIOutput)(COYOTE_SDI_2 | COYOTE_SDI_3) },
				{ COYOTE_PLAYER_4, COYOTE_SDI_4 },
			}
		}
	},
	{
		COYOTE_PSLAYOUT_C3,
		{
			"C3", COYOTE_PSLAYOUT_C3, (Player)(COYOTE_PLAYER_1 | COYOTE_PLAYER_2 | COYOTE_PLAYER_3),
			{
				{ COYOTE_PLAYER_1, COYOTE_SDI_1 },
				{ COYOTE_PLAYER_2, COYOTE_SDI_2 },
				{ COYOTE_PLAYER_3, (SDIOutput)(COYOTE_SDI_3 | COYOTE_SDI_4) },
			}
		}
	},
	{
		COYOTE_PSLAYOUT_F,
		{
			"F", COYOTE_PSLAYOUT_F, COYOTE_PLAYER_1,
			{
				{ COYOTE_PLAYER_1, (SDIOutput)(COYOTE_SDI_1 | COYOTE_SDI_2 | COYOTE_SDI_3 | COYOTE_SDI_4) },
			}
		}
	},
	{
		COYOTE_PSLAYOUT_E,
		{
			"E", COYOTE_PSLAYOUT_E, COYOTE_PLAYER_1,
			{
				{ COYOTE_PLAYER_1, (SDIOutput)(COYOTE_SDI_1 | COYOTE_SDI_2 | COYOTE_SDI_3 | COYOTE_SDI_4) },
			}
		}
		
	},
	{
		COYOTE_PSLAYOUT_D1,
		{
			"D1", COYOTE_PSLAYOUT_D1, (Player)(COYOTE_PLAYER_1 | COYOTE_PLAYER_4),
			{
				{ COYOTE_PLAYER_1, (SDIOutput)(COYOTE_SDI_1 | COYOTE_SDI_2 | COYOTE_SDI_3) },
				{ COYOTE_PLAYER_4, COYOTE_SDI_4 },
			}
		}
	},
	{
		COYOTE_PSLAYOUT_D2,
		{
			"D2", COYOTE_PSLAYOUT_D2, (Player)(COYOTE_PLAYER_1 | COYOTE_PLAYER_2),
			{
				{ COYOTE_PLAYER_1, COYOTE_SDI_1 },
				{ COYOTE_PLAYER_4, (SDIOutput)(COYOTE_SDI_2 | COYOTE_SDI_3 | COYOTE_SDI_4) },
			}
		}
	},
};

static const auto &LayoutNameMap
{
	[] (void)
	{
		std::map<std::string, Coyote::PresetLayout> RetVal;
		
		for (auto &Pair : Layouts)
		{
			assert(Pair.second.ID == Pair.first); //Check for typos while we're at it I guess
			
			RetVal.emplace(Pair.second.TextName, Pair.second.ID);
		}
		
		return RetVal;
	} () //Immediately call this lambda
};

static const std::array<Coyote::Output Coyote::Preset::*, COYOTE_MAX_OUTPUTS> PresetOutputStructs
{
	&Coyote::Preset::Output1,
	&Coyote::Preset::Output2,
	&Coyote::Preset::Output3,
	&Coyote::Preset::Output4,
};

Coyote::Player Coyote::GetPresetPlayers(const Coyote::Preset &Input)
{	
	Player RetVal = COYOTE_PLAYER_INVALID;
	
	Coyote::Player Inc = COYOTE_PLAYER_1;
	
	const LayoutInfo *const Info = LookupPresetLayoutByID(Input.Layout);
	
	if (!Info) return COYOTE_PLAYER_INVALID;
	
	for (const Coyote::Output Preset::*const &OutputPtr : PresetOutputStructs)
	{
		const Coyote::Output *const OutputObj = &(Input.*OutputPtr);
		
		if ((Info->Players & Inc) != 0 && OutputObj->Filename.GetLength() > 0)
		{
			RetVal = (Player)(RetVal | Inc);
		}
	
		Inc = (Player)(Inc << 1);
	}
	
	return RetVal;
}

Coyote::SDIOutput Coyote::GetPlayerSDIOutputs(const Coyote::Preset &Input, const Coyote::Player InPlayer)
{
	const LayoutInfo *const Info = LookupPresetLayoutByID(Input.Layout);
	
	if (!Info || !Info->SDIOuts.count(InPlayer)) return COYOTE_SDI_INVALID;
	
	return Info->SDIOuts.at(InPlayer);
}

Coyote::SDIOutput Coyote::GetPresetSDIOutputs(const Coyote::Preset &Input)
{	
	SDIOutput RetVal = COYOTE_SDI_INVALID;
	
	Coyote::Player Inc = COYOTE_PLAYER_1;
	
	const LayoutInfo *const Info = LookupPresetLayoutByID(Input.Layout);
	
	if (!Info) return COYOTE_SDI_INVALID;
	
	for (const Coyote::Output Preset::*const &OutputPtr : PresetOutputStructs)
	{
		const Coyote::Output *const OutputObj = &(Input.*OutputPtr);
		
		if ((Info->Players & Inc) != 0 && Info->SDIOuts.count(Inc) && OutputObj->Filename.GetLength() > 0)
		{
			RetVal = (SDIOutput)(RetVal | Info->SDIOuts.at(Inc));
		}
	
		Inc = (Player)(Inc << 1);
	}
	
	return RetVal;
}

const LayoutInfo *Coyote::LookupPresetLayoutByID(const Coyote::PresetLayout LayoutID)
{
	if (!Layouts.count(LayoutID)) return nullptr;
	
	return &Layouts.at(LayoutID);
}

const LayoutInfo *Coyote::LookupPresetLayoutByString(const std::string &LayoutName)
{
	if (!LayoutNameMap.count(LayoutName)) return nullptr;
	
	return &Layouts.at(LayoutNameMap.at(LayoutName));
}
