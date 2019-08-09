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

#ifndef __LIBCOYOTE_SESSION_H__
#define __LIBCOYOTE_SESSION_H__

#ifndef __cplusplus
#error "This is the C++ set of bindings, you're looking for libcoyote_c.h"
#endif //__cplusplus
#include <string>
#include <stdint.h>
#include <vector>
#include "macros.h"
#include "statuscodes.h"
#include "datastructures.h"

namespace Coyote
{
	class Session
	{
	private:
		void *Internal;
	public:
		static constexpr size_t DefaultCommandTimeoutSecs = 10;
		
		Session(const std::string &Host);
		Session(Session &&In);
		Session &operator=(Session &&In);
		
		//Disallow copying
		Session(const Session &) = delete;
		Session &operator=(const Session &) = delete;
		
		StatusCode GetAssets(std::vector<Coyote::Asset> &Out, const int32_t TimeoutMS = -1);
		StatusCode GetPresets(std::vector<Coyote::Preset> &Out, const int32_t TimeoutMS = -1);
		StatusCode GetTimeCode(Coyote::TimeCode &Out, const int32_t PK = 0);
		
		//Input parameter names must match the expected JSON names
		StatusCode Take(const int32_t PK = 0);
		StatusCode Pause(const int32_t PK = 0);
		StatusCode End(const int32_t PK = 0);
		StatusCode SeekTo(const int32_t PK, const uint32_t TimeIndex);
		StatusCode InstallAsset(const std::string &AssetPath);
		StatusCode DeleteAsset(const std::string &AssetName);
		StatusCode RenameAsset(const std::string &CurrentName, const std::string &NewName);
		StatusCode ReorderPresets(const int32_t PK1, const int32_t PK2);
		StatusCode DeletePreset(const int32_t PK);
		StatusCode CreatePreset(const Coyote::Preset &Ref);
		StatusCode UpdatePreset(const Coyote::Preset &Ref);
		StatusCode LoadPreset(const Coyote::Preset &Ref);
		StatusCode BeginUpdate(void);
		StatusCode IsUpdateDetected(bool &ValueOut); //Deprecated
		StatusCode GetDisks(std::vector<std::string> &Out);
		StatusCode EjectDisk(const std::string &DriveLetter);
		StatusCode GetHardwareState(Coyote::HardwareState &Out);
		StatusCode RestartService(void);
		StatusCode GetIP(const int32_t AdapterID, Coyote::NetworkInfo &Out);
		StatusCode SetIP(const Coyote::NetworkInfo &Input);
		StatusCode InitializeCoyote(const Coyote::ResolutionMode Resolution, const Coyote::RefreshMode RefreshRate);
		StatusCode SetHardwareMode(const Coyote::ResolutionMode Resolution, const Coyote::RefreshMode RefreshRate);
		StatusCode SelectPreset(const int32_t PK);
		StatusCode GetMediaState(Coyote::MediaState &Out);
		StatusCode GetServerVersion(std::string &Out);
		StatusCode DetectUpdate(bool &ValueOut, std::string *NewVersionOut = nullptr);
		StatusCode RebootCoyote(void);
		StatusCode ShutdownCoyote(void);
		StatusCode SoftRebootCoyote(void);
		StatusCode SelectNext(void);
		StatusCode SelectPrev(void);
		StatusCode TakeNext(void);
		StatusCode TakePrev(void);
		
		virtual ~Session(void);
		
		void SetCommandTimeoutSecs(const time_t TimeoutSecs = DefaultCommandTimeoutSecs);
		time_t GetCommandTimeoutSecs(void) const;
	};
	
}
#endif //__LIBCOYOTE_SESSION_H__
