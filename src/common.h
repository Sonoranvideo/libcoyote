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

#ifndef __LIBCOYOTE_COMMON_H__
#define __LIBCOYOTE_COMMON_H__

/**Headers we will likely use a lot.**/
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <memory>
#include <functional>
#include <typeinfo>
#include <chrono>
#include <thread>
#include <type_traits>

#ifdef LCVERBOSE //Only usable for C++
#define LDEBUG (std::cerr << "THREAD " << std::this_thread::get_id() << " Executing " << __func__ << "() line number " << __LINE__ << " in file " << __FILE__ << std::endl)
#define LDEBUG_MSG(Msg) (std::cerr << "THREAD " << std::this_thread::get_id() << " Executing " << __func__ << "() line number " << __LINE__ << " in file " << __FILE__ << std::endl << "MESSAGE \"" << Msg << '"' << std::endl)
#else
#define LDEBUG ((void)0)
#define LDEBUG_MSG(x) ((void)0)
#endif //LCVERBOSE

#else
#include <stdbool.h>
#endif //__cplusplus

#ifdef WIN32
#include <windows.h>
#define COYOTE_SLEEP(x) Sleep(x);
#else
#include <unistd.h>
#define COYOTE_SLEEP(x) usleep((x ) * 1000);
#endif //WIN32

template <typename MapType>
auto RebuildMapBackwards(const MapType &Original)
{
	auto Iter = Original.begin();
	
	std::map<decltype(Iter->second), decltype(Iter->first) > RetVal;
	
	for (; Iter != Original.end(); ++Iter)
	{
		RetVal.emplace(Iter->second, Iter->first);
	}
	
	return RetVal;
}

#endif //__LIBCOYOTE_COMMON_H__
