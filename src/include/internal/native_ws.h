#ifndef __LIBCOYOTE_NATIVE_WS_H__
#define __LIBCOYOTE_NATIVE_WS_H__

#include "common.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <libwebsockets.h>

namespace WS
{
	static constexpr uint16_t PortNum = 4488;
	
	class WSMessage
	{
	private:
		std::vector<uint8_t> Buffer;
	public:
	
		inline WSMessage(const std::vector<uint8_t> &Data)
		: Buffer()
		{
			this->SetContents(Data.data(), Data.size());
		}
	
		inline WSMessage(const void *Data = nullptr, const size_t DataLength = 0, const bool AutoPack = false)
		: Buffer()
		{
			if (Data && DataLength) AutoPack ? this->PackContents(Data, DataLength) : this->SetContents(Data, DataLength);
		}
		
		void SetContents(const void *Input = nullptr, const size_t Length = 0);
		void PackContents(const void *Input, const size_t Length);
		std::vector<uint8_t> GetCopy(void) const;
		uint8_t *GetDataHead(void);
		const uint8_t *GetConstData(void) const;
		uint8_t *GetPackedDataHead(void);
		const uint8_t *GetPackedDataHead(void) const;
		
		size_t GetSize(void) const;
		size_t GetPackedSize(void) const;
		void DeleteLeadingBytes(const uint64_t Bytes);
	};
	
	class WSMessageFragment
	{
	private:
		std::vector<uint8_t> Buffer;
		size_t RequiredSize;
	public:
		inline WSMessageFragment(const void *Data, const size_t DataSize, const size_t RequiredSize)
		: Buffer(), RequiredSize(RequiredSize)
		{
			this->Append(Data, DataSize);
		}
		
		inline bool Ready(void) const { return this->Buffer.size() == RequiredSize + sizeof(uint32_t); }
		
		inline void Append(const void *Data, const size_t DataSize)
		{
			const size_t OldSize = this->Buffer.size();
			
			this->Buffer.resize(OldSize + DataSize);
			
			memcpy(this->Buffer.data() + OldSize, Data, DataSize);
		}
		
		inline std::vector<uint8_t> Graduate(void)
		{
			return std::move(this->Buffer);
		}
	};
	
	class WSConnection
	{ //This class's interface MUST match its webassembly counterpart!
	private:
		static const struct lws_protocols WSProtocols[];

		bool (*OnReceiveCallback)(void*, WSMessage*);
		void *OnReceiveCallbackUserData;

		//Instance data members
		std::queue<WSMessage*> Incoming;
		std::queue<WSMessage*> Outgoing;
		
		std::mutex IMutex, OMutex;
		
		struct lws_context *Ctx;
		struct lws *Socket;
		
		std::thread *Thread;
		
		WSMessageFragment *RecvFragment;
		std::atomic_bool ShouldDie;
		
		//Private methods
		bool InitWebSockets(void);
		static int WSCallback(struct lws *WSDesc, const enum lws_callback_reasons Reason, void *UserData, void *Data, const size_t DataLength);
		void AddFragment(const void *Data, const size_t DataSize, const size_t RequiredSize = 0);
		
		inline bool AwaitingChunks(void) const { return this->RecvFragment; }
		
		void ThreadFunc(void);
		
		void KillThread(void);
		void Send(WSMessage *Msg);
		WSMessage *Recv(void);
	public:
		WSConnection(bool (*const OnReceiveCallback)(void*, WSMessage*), void *UserData);
		~WSConnection(void);
		bool Connect(const std::string &Host);
		void Shutdown(void);
		
		//No copying or moving
		WSConnection(const WSConnection &) = delete;
		WSConnection &operator=(const WSConnection &) = delete;
	};
}

#endif //__LIBCOYOTE_NATIVE_WS_H__
