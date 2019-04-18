#ifndef __LIBCOYOTE_CURLREQUESTS_H__
#define __LIBCOYOTE_CURLREQUESTS_H__

#include "common.h"

namespace CurlRequests
{
	enum CurlOperation : uint8_t
	{
		OPERATION_INVALID = 0,
		OPERATION_GET,
		OPERATION_POST,
		OPERATION_PUT,
		OPERATION_MAX
	};

	class CurlSession
	{
	private:
		static bool InitPerformed;
		
		void *CurlDescriptor;
		
		static size_t CurlWriteFunc(char *Ptr, size_t UnitSize, size_t NumUnits, std::vector<uint8_t> *Response);

	public:
		CurlSession(void);
		~CurlSession(void);
		
		bool SendJSON(const char *URL, const char *JSONData, std::vector<uint8_t> *Response = nullptr);
	};
}

#endif //__LIBCOYOTE_CURLREQUESTS_H__
