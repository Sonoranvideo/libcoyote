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

#ifndef __LIBCOYOTE_SESSION_H__
#define __LIBCOYOTE_SESSION_H__

#ifndef __cplusplus
#error "This library requires C++. You could also use the Python bindings, if you prefer, or generate your own bindings to libcoyote."
#endif //__cplusplus
#include "common.h"

#include <time.h>
#include "macros.h"
#include "statuscodes.h"
#include "datastructures.h"

namespace Coyote
{
	class EXPFUNC Session
	{
	private:
		void *Internal;
	public:
		static constexpr size_t DefaultCommandTimeoutSecs = 10;
		
		Session(const std::string &Host, const int NumAttempts = -1);
		Session(Session &&In);
		Session &operator=(Session &&In);
		
		//Disallow copying
		Session(const Session &) = delete;
		Session &operator=(const Session &) = delete;
		
		StatusCode GetAssets(std::vector<Asset> &Out);
		StatusCode GetPresets(std::vector<Preset> &Out);
		StatusCode GetPresetStates(std::vector<PresetState> &Out);
		StatusCode GetTimeCode(TimeCode &Out, const int32_t PK = 0);
		
		//Input parameter names must match the expected JSON names
		StatusCode Take(const int32_t PK = 0);
		StatusCode Pause(const int32_t PK = 0);
		StatusCode End(const int32_t PK = 0);
		StatusCode SeekTo(const int32_t PK, const uint32_t TimeIndex);
		StatusCode InstallAsset(const std::string &FullPath);
		StatusCode DeleteAsset(const std::string &FullPath);
		StatusCode RenameAsset(const std::string &FullPath, const std::string &NewName);
		StatusCode ReorderPresets(const int32_t PK1, const int32_t PK2);
		StatusCode DeletePreset(const int32_t PK);
		StatusCode CreatePreset(const Preset &Ref);
		StatusCode UpdatePreset(const Preset &Ref);
		StatusCode ReadAssetMetadata(const std::string &FullPath, AssetMetadata &Out);
		StatusCode BeginUpdate(void);
		StatusCode GetDisks(std::vector<Drive> &Out);
		inline StatusCode GetDrives(std::vector<Drive> &Out) { return this->GetDisks(Out); }
		StatusCode GetDiskAssets(std::vector<ExternalAsset> &Out, const std::string &DriveName, const std::string &Subpath = "");
		StatusCode EjectDisk(const std::string &Mountpoint);
		StatusCode GetUnitID(std::string &UnitIDOut, std::string &NicknameOut);
		StatusCode GetKonaHardwareState(KonaHardwareState &Out);
		StatusCode GetIP(const int32_t AdapterID, NetworkInfo &Out);
		StatusCode SetIP(const NetworkInfo &Input);
		StatusCode SetKonaHardwareMode(	const std::array<ResolutionMode, NUM_KONA_OUTS> &Resolutions,
										const RefreshMode RefreshRate,
										const HDRMode HDRMode = Coyote::COYOTE_HDR_DISABLED,
										const EOTFMode EOTFSetting = Coyote::COYOTE_EOTF_NORMAL,
										const bool ConstLumin = false,
										const KonaAudioConfig AudioConfig = COYOTE_KAC_DISABLED);
		StatusCode SelectPreset(const int32_t PK);
		StatusCode MovePreset(const int32_t PK, const std::string TabID, const uint32_t NewIndex);
		StatusCode GetServerVersion(std::string &Out);
		StatusCode DetectUpdate(bool &ValueOut, std::string *NewVersionOut = nullptr);
		StatusCode IsMirror(bool &ValueOut);
		StatusCode GetCurrentRole(UnitRole &RoleOut);
		StatusCode AddMirror(const std::string &MirrorIP);
		StatusCode GetMirrors(std::vector<Mirror> &Out);
		StatusCode GetUnitType(UnitType &Out);
		StatusCode GetEffectivePrimary(Mirror &Out);
		StatusCode GetDesignatedPrimary(Mirror &Out);
		StatusCode SynchronizerBusy(bool &Out);
		StatusCode DeconfigureSync(void);
		StatusCode RebootCoyote(void);
		StatusCode ShutdownCoyote(void);
		StatusCode SoftRebootCoyote(void);
		StatusCode SelectNext(void);
		StatusCode SelectPrev(void);
		StatusCode TakeNext(void);
		StatusCode TakePrev(void);
		StatusCode RenameGoto(const int32_t PK, const int32_t Time, const std::string &Name);
		StatusCode RenameCountdown(const int32_t PK, const int32_t Time, const std::string &Name);
		StatusCode DeleteGoto(const int32_t PK, const int32_t Time);
		StatusCode DeleteCountdown(const int32_t PK, const int32_t Time);
		StatusCode CreateGoto(const int32_t PK, const int32_t Time, const std::string &Name);
		StatusCode CreateCountdown(const int32_t PK, const int32_t Time, const std::string &Name);
		StatusCode GetLogsZip(std::vector<uint8_t> &OutBuffer);
		StatusCode ReadLog(const std::string &SpokeName, const int Year, const int Month, const int Day, std::string &LogOut);
		StatusCode ExportLogsZip(const std::string &Mountpoint);
		StatusCode SetPausedState(const int32_t PK, const bool Value);
		StatusCode SetPause(const int32_t PK);
		StatusCode UnsetPause(const int32_t PK);
		StatusCode SetUnitNickname(const std::string &Nickname);
		StatusCode RestartSpoke(const std::string &SpokeName);
		StatusCode KillSpoke(const std::string &SpokeName);
		StatusCode StartSpoke(const std::string &SpokeName);
		StatusCode GetGenlockSettings(Coyote::GenlockSettings &Out);
		StatusCode SetHorzGenlock(int32_t HorzValue);
		StatusCode SetVertGenlock(int32_t HorzValue);
		StatusCode GetSupportedSinks(std::vector<std::string> &Out);
		StatusCode GetSupportsS12G(bool &ValueOut);
		StatusCode GetIsServerUnit(bool &ValueOut);
		StatusCode GetBMDResolution(const uint32_t SDIIndex, Coyote::ResolutionMode &ResOut, Coyote::RefreshMode &FPSOut);
		StatusCode SetBMDResolution(const uint32_t SDIIndex, const Coyote::ResolutionMode Res, const Coyote::RefreshMode FPS);
		StatusCode DownloadState(std::string &PresetsJsonOut, std::string &SettingsJsonOut);
		StatusCode UploadState(const std::string &PresetsJson, const std::string &SettingsJson);
		StatusCode DeleteWatchPath(const std::string &Path);
		StatusCode AddWatchPath(const std::string &Path);
		StatusCode GetWatchPaths(std::vector<std::string> &Out);
		StatusCode ManualAddAsset(const std::string &FullPath);
		StatusCode ManualForgetAsset(const std::string &FullPath);
		StatusCode ExitSupervisor(void);
		
		virtual ~Session(void);
		
		void SetCommandTimeoutSecs(const time_t TimeoutSecs = DefaultCommandTimeoutSecs);
		time_t GetCommandTimeoutSecs(void) const;
		bool HasConnectionError(void) const;
		
		bool Connected(void) const;
		bool Reconnect(const std::string &Host = {});
		std::string GetHost(void) const;
		std::string GetHostOS(void) const;
		
		///WARNING: All callbacks will execute in a different thread than the thread calling the setter method!
		
		void SetPlaybackEventCallback(const PBEventCallback CB, void *const UserData = nullptr); 
		void SetStateEventCallback(const StateEventType EType, const StateEventCallback CB, void *const UserData = nullptr);
		
		//Only useful for Sonoran internal code.
		StatusCode _SVS_WriteCytLog_(const std::string &Param1, const std::string &Param2); //Users: DO NOT use this method.
		StatusCode _SVS_RegisterPing_(const std::string &Param1);
		StatusCode _SVS_RegisterReady_(const std::string &Param1);
	};
	
	EXPFUNC std::vector<LANCoyote> GetLANCoyotes(void);
}
#endif //__LIBCOYOTE_SESSION_H__
