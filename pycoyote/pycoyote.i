%module pycoyote
#pragma SWIG nowarn=503,325,362,509
%{
/* Includes the header in the wrapper code */
#include <src/include/coyotestring.h>
#include <src/include/statuscodes.h>
#include <src/include/datastructures_c.h>
#include <src/include/datastructures.h>
#   define SWIG_PYTHON_EXTRA_NATIVE_CONTAINERS 
#include <src/include/libcoyote.h>
#include <src/include/session.h>
%}

/* Parse the header file to generate wrappers */
%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"
%include "std_array.i"
%include "typemaps.i"
%include "stdint.i"
%include <src/include/coyotestring.h>
%include <src/include/statuscodes.h>
%include <src/include/datastructures_c.h>
%include <src/include/datastructures.h>

%apply std::string& OUTPUT {std::string &Out};
%apply bool& OUTPUT {bool &};

%extend Coyote::CoyoteString
{
	PyObject *__str__(void)
	{
		return PyUnicode_DecodeUTF8($self->GetStdString().c_str(), $self->GetStdString().length(), nullptr);
	}
	PyObject *__repr__(void)
	{
		return PyUnicode_DecodeUTF8($self->GetStdString().c_str(), $self->GetStdString().length(), nullptr);
	}	
};

namespace std
{
	%template(StringVector) std::vector<std::string>;
	%template(AssetVector) std::vector<Coyote::Asset>;
	%template(PresetVector) std::vector<Coyote::Preset>;
	%template(NetInfoVector) std::vector<Coyote::NetworkInfo>;
	%template(OutputsArray) std::array<int32_t, COYOTE_MAX_OUTPUTS>;
}


%include <src/include/libcoyote.h>
%include <src/include/session.h>
