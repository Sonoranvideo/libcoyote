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

/**It's important that you use MSGPACK_DEFINE_MAP, not MSGPACK_DEFINE or MSGPACK_DEFINE_ARRAY.
 *Communicator won't understand the data structure otherwise. It looks at items by named text field.**/

#ifndef __LIBCOYOTE_DATASTRUCTURES_H__
#define __LIBCOYOTE_DATASTRUCTURES_H__


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "common.h"

#include "macros.h"
#include "statuscodes.h"


typedef Coyote::ResolutionMode ResolutionMode;
typedef Coyote::RefreshMode RefreshMode;
//~ typedef Coyote::PresetLayout PresetLayout;
typedef Coyote::HDRMode HDRMode;
typedef Coyote::EOTFMode EOTFMode;

namespace Coyote
{
	typedef void (*PBEventCallback)(const PlaybackEventType EType, const int32_t PK, const int32_t Time, void *UserData);
	typedef void (*StateEventCallback)(const StateEventType EventType, void *UserData);


	
	struct Size2D //These don't inherit Object because there's no need and doing so creates a diamond of death.
	{
		int32_t Width, Height;
		
		inline bool operator==(const Size2D &Other) const { return this->Width == Other.Width && this->Height == Other.Height; }
		inline bool operator!=(const Size2D &Other) const { return !(*this == Other); }
		
		MSGPACK_DEFINE_MAP(Width, Height)
	};
	
	struct Coords2D
	{
		int32_t X, Y;
		
		inline bool operator==(const Coords2D &Other) const { return this->X == Other.X && this->Y == Other.Y; }
		inline bool operator!=(const Coords2D &Other) const { return !(*this == Other); }

		MSGPACK_DEFINE_MAP(X, Y)
	};
	
	struct Rect : public Size2D, public Coords2D
	{
		inline bool operator==(const Rect &Other) const
		{
			return	*static_cast<const Coords2D*>(this) == static_cast<const Coords2D&>(Other) &&
					*static_cast<const Size2D*>(this) == static_cast<const Size2D&>(Other);
		}
		
		inline bool operator!=(const Rect &Other) const { return !(*this == Other); }
		MSGPACK_DEFINE_MAP(Width, Height, X, Y)
	};
	
	struct Cube : public Rect
	{
		int32_t Z, Depth;

		inline bool operator==(const Cube &Other) const
		{
			return	*static_cast<const Rect*>(this) == static_cast<const Rect&>(Other) &&
					this->Z == Other.Z && this->Depth == Other.Depth;
		}
		
		inline bool operator!=(const Cube &Other) const { return !(*this == Other); }
		
		MSGPACK_DEFINE_MAP(Width, Height, X, Y, Z, Depth)
	};
	
	struct Object
	{
		virtual ~Object(void) = default;
	};
	
	enum class CanvasOrientationEnum
	{
		Invalid = 0,
		Custom = 1,
		Single = 2,
		Standard = 3,
		Landscape = 4,
		Portrait = 5
	};

	struct CanvasOrientation : public Object
	{
		Coords2D CustomCoords; //Only valid if Orientation == Custom.
		CanvasOrientationEnum Orientation;
		
		inline operator CanvasOrientationEnum(void) const { return this->Orientation; }
		
		inline bool operator==(const CanvasOrientation &Other) const
		{
			if (this->Orientation == CanvasOrientationEnum::Custom)
			{
				return this->Orientation == Other.Orientation && this->CustomCoords == Other.CustomCoords;
			}
			
			return this->Orientation == Other.Orientation;
		}
		
		inline bool operator!=(const CanvasOrientation &Other) const { return !(*this == Other); }
		inline bool operator==(const CanvasOrientationEnum &Other) const { return this->Orientation == Other; }
		inline bool operator!=(const CanvasOrientationEnum &Other) const { return this->Orientation != Other; }
		
		CanvasOrientation(const CanvasOrientationEnum Input = CanvasOrientationEnum::Invalid, const Coords2D *Coords = nullptr)
			: CustomCoords(Coords ? *Coords : Coords2D{}), Orientation(Input)
		{
		}
	};

	struct PinnedAsset : public Object
	{
		std::string FullPath;
		Cube PinnedCoords;
		float Hue, Saturation, Brightness, Contrast;
		MSGPACK_DEFINE_MAP(FullPath, PinnedCoords, Hue, Saturation, Brightness, Contrast)
	};
	
	struct ProjectorCanvasConfig : public Object
	{ //This cooresponds to a datatype used in Sonoran's video rendering core, Projector, aka Coyote Core.
		CanvasOrientation Orientation;
		Size2D Dimensions;
		uint32_t NumOutputs;
		
		inline ProjectorCanvasConfig(const CanvasOrientation Orientation, const Size2D Dimensions, const uint32_t NumOutputs)
			: Orientation(Orientation), Dimensions(Dimensions), NumOutputs(NumOutputs)
		{
		}
		
		inline ProjectorCanvasConfig(void) : Orientation(), Dimensions(), NumOutputs() {}
		
		inline bool operator==(const ProjectorCanvasConfig &Other) const { return this->Orientation == Other.Orientation && this->Dimensions == Other.Dimensions && this->NumOutputs == Other.NumOutputs; }
		inline bool operator!=(const ProjectorCanvasConfig &Other) const { return !(*this == Other); }
		
		MSGPACK_DEFINE_MAP(Dimensions, NumOutputs)
	};
	
	struct CanvasInfo : public Object
	{
		ProjectorCanvasConfig CanvasCfg; //Geometry and configuration for the surface we draw on.
		std::vector<PinnedAsset> Assets;
		SinkType SinkTypes;
		uint32_t Index;
		
		MSGPACK_DEFINE_MAP(Assets, SinkTypes, Index)
	};
	
	struct PresetMark : public Object
	{
		int32_t ID;
		int32_t TimeMS;
		std::string Name;
		
		MSGPACK_DEFINE_MAP(ID, TimeMS, Name)
	};
	
	struct LANCoyote : public Object
	{
		std::string APIVersion;
		std::string CommunicatorVersion;
		std::string GUID;
		std::string Nickname;
		std::string IP;
		UnitRole CurrentRole;
		UnitType Type;
		MSGPACK_DEFINE_MAP(APIVersion, CommunicatorVersion, GUID, Nickname, CurrentRole, Type)
	};
	
	struct TimeCode : public Object
	{
		int32_t PK;
		int32_t Time;
		std::vector<int32_t> VUData;
		
		MSGPACK_DEFINE_MAP(PK, Time, VUData)
	};
	
	struct Drive : public Object
	{
		std::string Mountpoint;
		int64_t Total, Used, Free;
		bool IsExternal;
		
		MSGPACK_DEFINE_MAP(Mountpoint, Total, Used, Free, IsExternal)
	};
	
	struct Asset : public Object
	{
		std::string FullPath; ///The part we care about most
		std::string Checksum;
		int64_t LastModified;
		int64_t TotalSize;
		int64_t CurrentSize;
		AssetState Status;
		
		MSGPACK_DEFINE_MAP(FullPath, Checksum, LastModified, TotalSize, CurrentSize, Status)
	};
	
	struct AssetMetadata : public Object
	{
		std::string FilePath;
		std::string VideoFormat;
		std::string AudioFormat;
		std::string ContainerFormat;
		Size2D Resolution;
		uint64_t TotalSize;
		uint32_t VBitrate;
		uint32_t ABitrate;
		uint32_t ASampleRate;
		uint32_t AChannels;
		uint32_t FPS;
		int32_t Duration;
		
		MSGPACK_DEFINE_MAP(
			FilePath,
			FPS,
			VideoFormat,
			AudioFormat,
			ContainerFormat,
			Resolution,
			TotalSize,
			VBitrate,
			ABitrate,
			ASampleRate,
			AChannels,
			Duration
		)
	};
	

	struct TabOrdering : public Object
	{
		std::string TabID;
		int32_t Index;
		
		inline TabOrdering(const std::string TabID = "", const int32_t Index = 0) : TabID(TabID), Index(Index) {}
	}; //Processed by msgpack manually
	
	struct ExternalAsset : public Object
	{
		std::string Filename;
		std::string FullPath;
		int64_t FileSize;
		bool IsDirectory;
		
		MSGPACK_DEFINE_MAP(Filename, FullPath, FileSize, IsDirectory)
	};
	
	struct PresetState : public Object
	{
		int32_t PK;
		int32_t TRT;
		uint32_t CurrentLoop;
		bool IsPlaying;
		bool IsPaused;
		bool IsSelected;
		
		MSGPACK_DEFINE_MAP(PK, TRT, CurrentLoop, IsPlaying, IsPaused, IsSelected)
	};
	
	struct EXPFUNC Preset : public Object
	{
		std::string Name;
		std::string Notes;
		std::string Color;
		std::vector<CanvasInfo> Canvases; //Processed by a custom msgpack handler.
		std::unordered_map<int32_t, PresetMark> Gotos;
		std::unordered_map<int32_t, PresetMark> Countdowns;
		std::vector<uint16_t> Volume;
		std::map<std::string, TabOrdering> TabDisplayOrder; //Custom msgpack handler
		std::unordered_map<std::string, std::string> SinkOptions;
		
		int32_t PK;
		int32_t Loop;
		int32_t Link;
		int32_t DissolveInMS;
		int32_t DissolveOutMS;
		int32_t FadeInMS;
		int32_t FadeOutMS;
		int32_t InPosition;
		int32_t OutPosition;
		int32_t Dissolve;
		bool FreezeAtEnd;
		
		MSGPACK_DEFINE_MAP(
			Name,
			Notes,
			Color,
			Gotos,
			Countdowns,
			PK,
			Loop,
			Link,
			DissolveInMS,
			DissolveOutMS,
			FadeInMS,
			FadeOutMS,
			InPosition,
			OutPosition,
			Dissolve,
			FreezeAtEnd,
			Volume,
			SinkOptions
		)
		
		
		//Methods
		Coyote::Player GetPlayersForCanvas(const uint32_t Index) const; //If the preset can't be drawn by our official GUI, don't use these methods!
		Coyote::Player GetActivePlayerForCanvas(const uint32_t Index) const;
		const CanvasInfo *LookupCanvasByPlayer(const Coyote::Player PlayerNum) const;
		std::array<uint32_t, 2> *GetPlayerRangeForCanvas(const uint32_t Index) const;
		bool HasValidSinks(void) const;
	};
	
	struct KonaHardwareState : public Object
	{
		std::array<ResolutionMode, NUM_KONA_OUTS> Resolutions;
		RefreshMode RefreshRate;
		enum HDRMode HDRMode;
		EOTFMode EOTFSetting;
		KonaAudioConfig AudioConfig;
		bool ConstLumin;
		
		MSGPACK_DEFINE_MAP(
			//Resolution, !! Handled custom !!
			//RefreshRate, !! Handled custom !!
			HDRMode,
			EOTFSetting,
			ConstLumin
			//AudioConfig, !! Handled custom !!
		)
	};
	
	struct NetworkInfo : public Object
	{
		std::string IP;
		std::string Subnet;
		int32_t AdapterID;
		bool CableConnected;
		
		MSGPACK_DEFINE_MAP(IP, Subnet, AdapterID, CableConnected)
	};

	struct Mirror : public Object
	{
		std::string UnitID;
		std::string IP;
		UnitType Type;
		bool Busy;
		bool IsAlive;
		
		MSGPACK_DEFINE_MAP(UnitID, IP, Type, Busy, IsAlive)
	};
	
	struct GenlockSettings : public Object
	{
		std::string FormatString;
		int32_t HorzValue;
		int32_t VertValue;
		bool Genlocked;
		
		MSGPACK_DEFINE_MAP(FormatString, HorzValue, VertValue, Genlocked)
	};

	struct LicensingStatus : public Object
	{
		std::string LicenseKey;
		std::string ProductName;
		std::string LicenseMachineUUID;
		std::string UserName;
		LicensingCapabilities LicenseCaps;
		uint32_t UsedSeats;
		uint32_t TotalSeats;
		bool ValidLicense;

		MSGPACK_DEFINE_MAP(LicenseKey, ProductName, UserName, LicenseMachineUUID, LicenseCaps, UsedSeats, TotalSeats, ValidLicense)
	};

}

MSGPACK_ADD_ENUM(Coyote::CanvasOrientationEnum)

#endif //__LIBCOYOTE_DATASTRUCTURES_H__
