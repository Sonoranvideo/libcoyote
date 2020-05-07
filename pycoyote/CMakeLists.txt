cmake_minimum_required(VERSION 2.6)
project("pycoyote")
cmake_policy(SET CMP0003 NEW)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../src ${CMAKE_CURRENT_BINARY_DIR}/libcoyote)

#This CMake config will only work on MinGW or POSIX. Don't bother trying to build with MSVC.

set(EXTRA_CXXFLAGS)

if(DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -pedantic -Wall -g3 -O0 -fstack-protector-all ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -pedantic -Wall -O3 -fomit-frame-pointer ${EXTRA_CXXFLAGS})
endif()

find_package(PkgConfig)

set(sourcefiles pycoyote.cpp)

add_library(pycoyote SHARED ${sourcefiles})

set(EXTRA_LD)
if (WIN32)
	set(EXTRA_LD ${EXTRA_LD} -lws2_32)
	set_target_properties(pycoyote PROPERTIES SUFFIX ".pyd")
	if (NOT PYLIBS)
		set(PYLIBS -L/c/Coyote/Runtime/Python3/ -lpython38)
		set(PYHEADERS /c/Coyote/Runtime/Python3/include/)
	endif()
else()
	pkg_check_modules(PYTHON3 REQUIRED python3)
	
	set(PYLIBS ${PYTHON3_LIBRARIES})
	set(PYHEADERS ${PYTHON3_INCLUDE_DIRS})
endif()

set_target_properties(pycoyote PROPERTIES PREFIX "")
target_link_directories(pycoyote PUBLIC "../build/" "../winbuild")
target_link_libraries(pycoyote coyote ${PYLIBS} ${EXTRA_LD} -Wl,-R -Wl,. -Wl,-R -Wl,../build -Wl,-R -Wl,../winbuild)
target_include_directories(pycoyote PUBLIC ${PYHEADERS})

#target_compile_options(pycoyote PUBLIC ${PYTHON3_CFLAGS_OTHER})
