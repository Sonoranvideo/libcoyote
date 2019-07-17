#ifndef __LIBCOYOTE_COYOTESTRING_H__
#define __LIBCOYOTE_COYOTESTRING_H__
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>
namespace Coyote
{
	class CoyoteString
	{ //Keep this class as standard layout because we pass it to C.
	private:
		char *Buffer;
	public:
		inline CoyoteString(const char *String = "") : Buffer((char*)calloc(strlen(String) + 1, 1)) { strcpy(this->Buffer, String); }
		inline CoyoteString(const std::string &String) : CoyoteString(String.c_str()) {}
		inline ~CoyoteString(void) { free(this->Buffer); }
		inline operator const char*(void) const { return this->Buffer; }
		inline operator std::string(void) const { return this->Buffer; }
		inline const char *GetCString(void) const { return this->Buffer; }
		inline std::string GetStdString(void) const { return this->Buffer; }
		inline CoyoteString(const CoyoteString &Ref) : Buffer(strdup(Ref.Buffer)) {}
		inline CoyoteString(CoyoteString &&Ref) : Buffer(Ref.Buffer) { Ref.Buffer = nullptr; }
		inline CoyoteString &operator=(const CoyoteString &Ref)
		{
			if (this == &Ref) return *this;
			
			free(this->Buffer);
			
			this->Buffer = strdup(Ref.Buffer);
			return *this;
		}

		inline CoyoteString &operator=(CoyoteString &&Ref)
		{
			if (this == &Ref) return *this;
			
			free(this->Buffer);
			
			this->Buffer = Ref.Buffer;
			
			Ref.Buffer = nullptr;
			return *this;
		}
	};
}
#endif //__LIBCOYOTE_COYOTESTRING_H__
