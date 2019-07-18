cmake_minimum_required(VERSION 2.4)
project("LibCoyote")


set(EXTRA_CXXFLAGS)
set(EXTRA_LD "")

if (DEBUG)
	add_link_options(-fstack-protector)
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -g3 -O0 -Wno-deprecated-declarations -Wno-unused-function -fstack-protector ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -O3 -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS})
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-R  -Wl,.")

find_package(PkgConfig)

message("== Searching for libwebsockets...")

pkg_check_modules(LIBWS REQUIRED libwebsockets)

message("== Ok, found libwebsockets.")

INCLUDE (CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX(msgpack.hpp MSGPACK_HEADER)

if (NOT MSGPACK_HEADER)
	message(FATAL_ERROR "msgpack-c was not detected. Only the header files are required for the C++ version, but they're missing.")
endif()

set(sourcefiles msgpackproc.cpp session.cpp asyncmsgs.cpp asynctosync.cpp subscriptions.cpp native_ws.cpp)

add_library(coyote SHARED ${sourcefiles})
add_library(coyote_static STATIC ${sourcefiles})



if (WIN32)
	set(EXTRA_LD ${EXTRA_LD} -lws2_32)
endif()

target_link_libraries(coyote websockets ${EXTRA_LD})
