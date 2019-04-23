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

#ifndef __LIBCOYOTE_JSONPROC_H__
#define __LIBCOYOTE_JSONPROC_H__

#include "common.h"
#include "statuscodes.h"
#include "datastructures.h"
#include <json/json.h>

namespace JsonProc
{
	const std::string MkGetAssets(void);
	const std::string MkGetPresets(void);
	const std::string MkGetTimeCode(void);
	const std::string MkGetHardwareState(void);
	Json::Value CreateJsonMsg(const std::string &CommandName, const std::map<std::string, Json::Value> *Values = nullptr);
	Coyote::StatusCode DecodeAssets(const std::string &JSON, std::vector<Coyote::Asset> &Out);
}

#endif //__LIBCOYOTE_JSONPROC_H__
