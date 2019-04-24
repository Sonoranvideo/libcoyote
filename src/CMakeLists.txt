cmake_minimum_required(VERSION 2.4)
project("LibCoyote")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	add_compile_options(/std:c++14)
else()
	add_compile_options(-std=gnu++14 -pedantic -Wall -g3 -O0 -Wno-deprecated-declarations -Wno-unused-function)
endif()

set(sourcefiles curlrequests.cpp jsonproc.cpp session.cpp)

add_library(coyote SHARED ${sourcefiles})
add_library(coyote_static STATIC ${sourcefiles})

target_link_libraries(coyote curl jsoncpp)
