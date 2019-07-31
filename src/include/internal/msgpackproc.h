#ifndef __LIBCOYOTE_MSGPACKPROC_H__
#define __LIBCOYOTE_MSGPACKPROC_H__

#define MSGPACK_DEFAULT_API_VERSION 2

#include "common.h"
#include "../datastructures.h"
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
