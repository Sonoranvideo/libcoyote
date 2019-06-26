#ifndef __LIBCOYOTE_MSGPACKPROC_H__
#define __LIBCOYOTE_MSGPACKPROC_H__

#define MSGPACK_DEFAULT_API_VERSION 2

#include "common.h"
#include "../datastructures.h"
#include <msgpack.hpp>

namespace MsgpackProc
{
	msgpack::object PackCoyoteObject(const Coyote::BaseObject *Object, msgpack::packer<msgpack::sbuffer> *Pack = nullptr);
	Coyote::BaseObject *UnpackCoyoteObject(const msgpack::object &Object, const std::type_info &Expected);
	void InitOutgoingMsg(msgpack::packer<msgpack::sbuffer> &Pack, const std::string &CommandName, const uint64_t MsgID, const msgpack::object *Values);
	std::map<std::string, msgpack::object> InitIncomingMsg(const void *Data, const size_t DataLength, uint64_t *MsgIDOut = nullptr);
}
#endif //__LIBCOYOTE_MSGPACKPROC_H__
