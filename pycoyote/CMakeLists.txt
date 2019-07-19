cmake_minimum_required(VERSION 2.4)
project("pycoyote")


set(EXTRA_CXXFLAGS)

if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -Wextra -g3 -O0 -Wno-deprecated-declarations -Wno-unused-function -fno-omit-frame-pointer -fstack-protector-all ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -O3 -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS})
endif()



find_package(PkgConfig)
find_package(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})


if (WIN32)
	set(PYLIBDIR /c/Coyote/Runtime/Python3/)
	set(PYHEADERS /c/Coyote/Runtime/Python3/include/)
else()
	pkg_check_modules(PYTHON3 REQUIRED python3)
	set(PYLIBDIR ${PYTHON3_LIBRARY_DIRS})
	set(PYHEADERS ${PYTHON3_INCLUDE_DIRS})
endif()

include_directories(${PYHEADERS} "../.." "..")
link_directories("../../build" "../build" "." ${PYLIBDIR})

set(swigspec pycoyote.i)

set_property(SOURCE ${swigspec} PROPERTY CPLUSPLUS ON)
set_property(SOURCE ${swigspec} PROPERTY SWIG_FLAGS "-py3;-nofastunpack;-threads;-castmode")

SWIG_ADD_MODULE(pycoyote python ${swigspec})
set_source_files_properties({swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS "-I{PYHEADERS}")

SWIG_LINK_LIBRARIES(pycoyote -L${PYLIBDIR} -lpython37 -lcoyote -Wl,-R -Wl,. ${EXTRA_LD})

set(EXTRA_LD "")

#target_link_libraries(coyote coyote ${EXTRA_LD})
