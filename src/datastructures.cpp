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

	for (; CurPlayer <= COYOTE_PLAYER_MAXVALUE && !(CurPlayer & Players); CurPlayer = (Coyote::Player)(CurPlayer << 1));

	return CurPlayer;
}

std::array<uint32_t, 2> *Coyote::GetPlayerRange(const Coyote::Player Players)
{ //1 based indexing, inclusive range. Used for GUI display purposes.
	if (Players == COYOTE_PLAYER_INVALID) return nullptr;

	std::unique_ptr<std::array<uint32_t, 2>> RetVal { new std::array<uint32_t, 2>{} };

	uint32_t &StartOut = RetVal->at(0);
	uint32_t &EndOut = RetVal->at(1);

	Coyote::Player CurPlayer = COYOTE_PLAYER_1;

	StartOut = 0;

	for (; CurPlayer <= COYOTE_PLAYER_MAXVALUE && !(CurPlayer & Players); ++StartOut, CurPlayer = (Coyote::Player)(CurPlayer << 1));

	EndOut = StartOut;
	//Ordering is not a mistake
	++StartOut;

	for (; CurPlayer <= COYOTE_PLAYER_MAXVALUE && (CurPlayer & Players) == CurPlayer; ++EndOut, CurPlayer = (Coyote::Player)(CurPlayer << 1));

	return RetVal.release();
}


std::array<uint32_t, 2> *Coyote::Preset::GetPlayerRangeForCanvas(const uint32_t Index) const
{ //1 based indexing, inclusive range. Used for GUI display purposes.
	const Coyote::Player Players = this->GetPlayersForCanvas(Index);

	return GetPlayerRange(Players);
}

const Coyote::CanvasInfo *Coyote::Preset::LookupCanvasByPlayer(const Coyote::Player PlayerNum) const
{
	for (uint32_t Inc = 0u; Inc < this->Canvases.size(); ++Inc)
	{
		if (this->GetPlayersForCanvas(Inc) & PlayerNum)
		{
			return &this->Canvases.at(Inc);
		}
	}

	return nullptr;
}

std::vector<uint32_t> Coyote::PlayersToIntegers(const Coyote::Player Players)
{
	std::vector<uint32_t> RetVal;

	Coyote::Player CurPlayer = COYOTE_PLAYER_1;
	
	uint32_t Inc = 0u;
	
	for (; CurPlayer <= COYOTE_PLAYER_MAXVALUE; ++Inc, CurPlayer = (Coyote::Player)(CurPlayer << 1))
	{
		if (!(CurPlayer & Players)) continue;
		
		RetVal.emplace_back(Inc);
	}
	
	return RetVal;
}

