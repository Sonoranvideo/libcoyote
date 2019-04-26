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

#ifndef __LIBCOYOTE_DATASTRUCTURES_H__
#define __LIBCOYOTE_DATASTRUCTURES_H__

#include <stdint.h>
#include <string>
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
		inline CoyoteString &operator=(const CoyoteString &Ref) { if (this == &Ref) return *this; this->Buffer = strdup(Ref.Buffer); return *this; }
		inline CoyoteString &operator=(CoyoteString &&Ref) { if (this == &Ref) return *this; this->Buffer = Ref.Buffer; Ref.Buffer = nullptr; return *this; }
	};
}

extern "C" //Not actually necessary at the moment but good practice anyways.
{
#include "datastructures_c.h"
}

namespace Coyote
{
	static_assert(std::is_standard_layout<CoyoteString>::value && alignof(char*) == alignof(CoyoteString), "CoyoteString is no longer a standard layout class due to some modification you made.");
	
	struct BaseObject
	{
		virtual ~BaseObject(void) = default;
	};
	
	
	struct TimeCode : public BaseObject, public Coyote_TimeCode
	{		
		inline operator uint32_t(void) const { return this->Time; }
	};
	
	struct Asset : public BaseObject, public Coyote_Asset
	{				
		inline operator const char*(void) const { return this->FileName; }
		inline operator std::string(void) const { return this->FileName; }
	};
	
	struct Output : public BaseObject, public Coyote_Output
	{
	};
	
	struct Preset : public BaseObject, public Coyote_Preset
	{
		Output Output1;
		Output Output2;
		Output Output3;
		Output Output4;
		
		inline operator int32_t(void) const { return this->PK; }
		inline Preset(void) : BaseObject(), Coyote_Preset()
		{
			this->Output_1 = &this->Output1;
			this->Output_2 = &this->Output2;
			this->Output_3 = &this->Output3;
			this->Output_4 = &this->Output4;
		}
	};
	
	struct HardwareState : public BaseObject, public Coyote_HardwareState
	{
	};
	
	struct NetworkInfo : public BaseObject, public Coyote_NetworkInfo
	{
	};
}
#endif //__LIBCOYOTE_DATASTRUCTURES_H__
