cmake_minimum_required(VERSION 2.4)
project("pycoyote")


set(EXTRA_CXXFLAGS)

if (DEBUG)
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -g3 -O0 -Wno-deprecated-declarations -Wno-unused-function ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -O3 -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS})
endif()

find_package(PkgConfig)
find_package(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})
pkg_check_modules(PYTHON3 REQUIRED python3)
include_directories(${PYTHON3_INCLUDE_DIRS} "../.." "..")
link_directories("../../build" "../build" ".")

set(swigspec pycoyote.i)

set_property(SOURCE ${swigspec} PROPERTY CPLUSPLUS ON)

SWIG_ADD_MODULE(pycoyote python ${swigspec})
set_source_files_properties({swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS "-I{PYTHON3_INCLUDE_DIRS}")

SWIG_LINK_LIBRARIES(pycoyote ${PYTHON3_LIBRARIES} -lcoyote -Wl,-R -Wl,. ${EXTRA_LD})

set(EXTRA_LD "")

#target_link_libraries(coyote coyote ${EXTRA_LD})
