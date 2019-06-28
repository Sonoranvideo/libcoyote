cmake_minimum_required(VERSION 2.4)
project("LibCoyote")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_compile_options(/std:c++14)
else()
	add_compile_options(-std=gnu++14 -pedantic -Wall -g3 -O0 -Wno-deprecated-declarations -Wno-unused-function)
endif()

find_package(PkgConfig)

message("== Searching for libwebsockets...")

pkg_check_modules(LIBWS REQUIRED libwebsockets)

message("== Ok, found libwebsockets.")

INCLUDE (CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX(msgpack.hpp MSGPACK_HEADER)

if (NOT MSGPACK_HEADER)
	message(FATAL_ERROR "msgpack-c was not detected. Only the header files are required for the C++ version, but they're missing.")
endif()

set(sourcefiles msgpackproc.cpp session.cpp asynctosync.cpp native_ws.cpp)

add_library(coyote SHARED ${sourcefiles})
add_library(coyote_static STATIC ${sourcefiles})

target_link_libraries(coyote websockets)
