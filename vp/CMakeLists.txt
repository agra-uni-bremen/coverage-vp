cmake_minimum_required(VERSION 2.8.12)
project(riscv-vp)

option(USE_SYSTEM_SYSTEMC "use systemc version provided by the system" OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter") # TODO: -Wpedantic
set(CMAKE_CXX_FLAGS_DEBUG "-g3")        #"-fsanitize=address -fno-omit-frame-pointer"
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Allows running tests without invoking `make install` first.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package( Boost REQUIRED COMPONENTS iostreams program_options log)
find_package(nlohmann_json REQUIRED)

if(NOT USE_SYSTEM_SYSTEMC)
	find_package( SystemC REQUIRED)
	include_directories( ${SystemC_INCLUDE_DIRS})
else()
	find_library(SystemC_LIBRARIES libsystemc.a)
endif()

find_package( SoftFloat REQUIRED )
include_directories( ${SoftFloat_INCLUDE_DIRS} )

subdirs(src)
