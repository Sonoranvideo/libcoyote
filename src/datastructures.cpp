/*
   Copyright 2021 Sonoran Video Systems

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

#include "include/common.h"
#include "include/datastructures.h"


Coyote::Player Coyote::Preset::GetPlayersForCanvas(const uint32_t Index) const
{ //Unless you also work for Sonoran, you probably shouldn't care about/bother with players. They're an illusion for UX.
	if (Index >= this->Canvases.size()) return Coyote::COYOTE_PLAYER_INVALID; //Bad index
	
	uint32_t UsedPlayers = 0u;
	
	for (uint32_t Inc = 0u; Inc < Index; ++Inc)
	{
		const CanvasInfo &Canvas { this->Canvases.at(Inc) };
		
		UsedPlayers += Canvas.CanvasCfg.NumOutputs;
	}
	
	if (UsedPlayers > NUM_KONA_OUTS) return Coyote::COYOTE_PLAYER_INVALID;

	const CanvasInfo &Canvas { this->Canvases.at(Index) };
	
	Coyote::Player TotalPlayers = Coyote::COYOTE_PLAYER_INVALID;

	//Skip to the first player for this canvas.
	Coyote::Player CurPlayer = static_cast<Coyote::Player>(Coyote::COYOTE_PLAYER_1 << UsedPlayers);
	
	for (uint32_t Inc = 0u; Inc < Canvas.CanvasCfg.NumOutputs; ++Inc, CurPlayer = (Coyote::Player)(CurPlayer << 1))
	{
		TotalPlayers = (Coyote::Player)(TotalPlayers | CurPlayer);
	}
	
	return TotalPlayers;
}

Coyote::Player Coyote::Preset::GetActivePlayerForCanvas(const uint32_t Index) const
{
	const Coyote::Player Players = this->GetPlayersForCanvas(Index);
	
	Coyote::Player CurPlayer = Coyote::COYOTE_PLAYER_1;
	
	for (; !(CurPlayer & Players); CurPlayer = (Coyote::Player)(CurPlayer << 1));
	
	return CurPlayer;
}
