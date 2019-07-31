cmake_minimum_required(VERSION 2.6)
project("LibCoyote")


set(EXTRA_CXXFLAGS)
set(EXTRA_LD "")

if (DEBUG)
	add_link_options(-fstack-protector-all)
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -g3 -O0 -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${EXTRA_CXXFLAGS})
else()
	add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -O3 -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS})
endif()

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-R  -Wl,.")

message("== Searching for Qt5WebSockets...")
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS WebSockets Core)

message("== Ok, found Qt5WebSockets")

file(GLOB HEADERS "*.h")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
INCLUDE (CheckIncludeFileCXX)

CHECK_INCLUDE_FILE_CXX(msgpack.hpp MSGPACK_HEADER)

if (NOT MSGPACK_HEADER)
	message(FATAL_ERROR "msgpack-c was not detected. Only the header files are required for the C++ version, but they're missing.")
endif()

set(sourcefiles msgpackproc.cpp session.cpp asyncmsgs.cpp asynctosync.cpp subscriptions.cpp native_ws.cpp)
if (NOT NO_C_API)
	set(sourcefiles ${sourcefiles} cinterface.cpp)
endif()

add_library(coyote SHARED ${sourcefiles})
add_library(coyote_static STATIC ${sourcefiles})


qt5_use_modules(coyote Core WebSockets)
qt5_use_modules(coyote_static Core WebSockets)
qt5_wrap_cpp(sourcefiles "${CMAKE_SOURCE_DIR}/include/internal/native_ws.h")

#set(CMAKE_AUTOMOC_MOC_OPTIONS "-I${CMAKE_SOURCE_DIR}/include/internal;-p;${CMAKE_SOURCE_DIR}/include/internal")

if (WIN32)
	set(EXTRA_LD ${EXTRA_LD} -lws2_32)
endif()

target_link_libraries(coyote ${EXTRA_LD})
