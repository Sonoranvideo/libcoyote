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
#ifndef __LIBCOYOTE_MSGPACKPROC_H__
#define __LIBCOYOTE_MSGPACKPROC_H__

#define MSGPACK_DEFAULT_API_VERSION 2

#include "common.h"
#include "include/datastructures.h"
#include <msgpack.hpp>

namespace MsgpackProc
{
	extern thread_local msgpack::zone &Zone; //This should stay at the top here, or close to it

	msgpack::object PackCoyoteObject(const Coyote::BaseObject *Object, msgpack::packer<msgpack::sbuffer> *Pack = nullptr);
	Coyote::BaseObject *UnpackCoyoteObject(const msgpack::object &Object, const std::type_info &Expected);
	void InitOutgoingMsg(msgpack::packer<msgpack::sbuffer> &Pack, const std::string &CommandName, const uint64_t MsgID = 0u, const msgpack::object *Values = nullptr);
	std::map<std::string, msgpack::object> InitIncomingMsg(const void *Data, const size_t DataLength, uint64_t *MsgIDOut = nullptr);
	
	template<typename T>
	msgpack::object STLArrayToMsgpackArray(const T &Vector)
	{ //Works on std::array, std::vector, and probably std::list
		msgpack::sbuffer Buffer;
		msgpack::packer<msgpack::sbuffer> Pack { Buffer };
		
		Pack.pack_array(Vector.size());
		
		for (auto &Element : Vector)
		{
			Pack.pack(Element);
		}
		
		return msgpack::unpack(Zone, static_cast<const char*>(Buffer.data()), Buffer.size());
	}
	
	template<typename T>
	msgpack::object STLMapToMsgpackMap(const T &Map)
	{
		msgpack::sbuffer Buffer;
		msgpack::packer<msgpack::sbuffer> Pack { Buffer };
		
		Pack.pack_map(Map.size());
		
		for (auto Iter = Map.begin(); Iter != Map.end(); ++Iter)
		{
			Pack.pack(Iter->first);
			Pack.pack(Iter->second);
		}
		
		return msgpack::unpack(Zone, static_cast<const char*>(Buffer.data()), Buffer.size());	
	}

}
#endif //__LIBCOYOTE_MSGPACKPROC_H__
