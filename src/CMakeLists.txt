cmake_minimum_required(VERSION 3.1.0)
project("LibCoyote")


set(EXTRA_CXXFLAGS)
set(EXTRAINCLUDE)
set(EXTRA_LD "")

if (LCVERBOSE)
	add_compile_definitions(LCVERBOSE)
endif()

if (DEBUG)
	add_compile_definitions(DEBUG)
endif()

if (NOT MSVC)
	if (DEBUG)
		add_link_options(-fstack-protector-all)
		add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -g3 -O0 -Wno-deprecated-declarations -Wno-unused-function -fstack-protector-all -fno-omit-frame-pointer ${EXTRA_CXXFLAGS})
	else()
		add_compile_options(-std=gnu++14 -fPIC -pedantic -Wall -O3 -Wno-deprecated-declarations -Wno-unused-function -fomit-frame-pointer ${EXTRA_CXXFLAGS})
	endif()
	
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-R  -Wl,.")
endif()

if (APPLE)
	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/opt/Coyote/Runtime/homebrew/opt/qt5" "/usr/local/opt/qt5/" "/opt/homebrew/opt/qt5/" )
	set(EXTRAINCLUDE ${EXTRAINCLUDE} "/opt/Coyote/Runtime/homebrew/include" "/opt/homebrew/include" "/usr/local/include" "/opt/local/include")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
message("== Searching for Qt5WebSockets...")
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS WebSockets Core)

message("== Ok, found Qt5WebSockets")

set(sourcefiles datastructures.cpp msgpackproc.cpp session.cpp asyncmsgs.cpp asynctosync.cpp subscriptions.cpp native_ws.cpp discovery.cpp easycanvasalign.cpp)
add_compile_options(-DMSGPACK_NO_BOOST)

add_library(coyote SHARED ${sourcefiles})
add_library(coyote_static STATIC EXCLUDE_FROM_ALL ${sourcefiles})

set_property(TARGET coyote_static PROPERTY CXX_STANDARD 14)
set_property(TARGET coyote_static PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET coyote PROPERTY CXX_STANDARD 14)
set_property(TARGET coyote PROPERTY CXX_STANDARD_REQUIRED ON)

qt5_use_modules(coyote Core WebSockets)
qt5_use_modules(coyote_static Core WebSockets)
#qt5_wrap_cpp(sourcefiles "${CMAKE_SOURCE_DIR}/include/internal/native_ws.h")


if (WIN32 AND NOT MSVC)
	set(EXTRA_LD ${EXTRA_LD} -lws2_32)
endif()

target_include_directories(coyote PUBLIC ${LIBCOYOTE_INCLUDES})

target_link_libraries(coyote ${EXTRA_LD})
target_include_directories(coyote PUBLIC ${EXTRAINCLUDE} "${CMAKE_CURRENT_LIST_DIR}/../msgpack-c/include")
