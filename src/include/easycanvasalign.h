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

#ifndef __LIBCOYOTE_ECA_H__
#define __LIBCOYOTE_ECA_H__

#include "common.h"
#include "statuscodes.h"
#include "datastructures.h"

namespace Coyote
{
	EXPFUNC bool AttachAssetToCanvas(const EasyCanvasAlignment Align, const std::string &FilePath, const Size2D &AssetDimensions, CanvasInfo &Canvas); //You need to set a Z coordinate yourself if you're using more than one asset on a canvas.
}
#endif //__LIBCOYOTE_ECA_H__
