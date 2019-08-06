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
#ifndef __LIBCOYOTE_COYOTESTRING_H__
#define __LIBCOYOTE_COYOTESTRING_H__
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>

#ifndef __cplusplus
#error "Included CoyoteString class in a C project!"
#endif //__cplusplus

namespace Coyote
{
	class CoyoteString
	{ //Keep this class as standard layout because we pass it to C.
	private:
		char *Buffer;
	public:
		inline CoyoteString(const char *String = "") : Buffer((char*)calloc(strlen(String) + 1, 1))
		{
			strcpy(this->Buffer, String);
		}
		inline CoyoteString(const std::string &String) : CoyoteString(String.c_str()) {}
		inline ~CoyoteString(void)
		{
			free(this->Buffer);
			this->Buffer = nullptr; //Increases the likelihood of catching a use-after-free
		}
		
		inline operator const char*(void) const { return this->Buffer; }
		inline operator std::string(void) const { return this->Buffer; }
		inline const char *GetCString(void) const { return this->Buffer; }
		inline std::string GetStdString(void) const { return this->Buffer; }
		inline size_t GetLength(void) const { return strlen(this->Buffer); }
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
