cmake_minimum_required(VERSION 2.8)

project(osu-performance)

set(BIN ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# Tells the compilers to put the binaries in the BIN folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN})

# Set a default configuration if none was specified
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "No release type specified. Setting to 'Release'.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

# Necessary so all shared source files can have the unified #include <Shared.h> statement.
# MSVC would complain if some use "../Shared" and some "Shared" without having those exact names specified
# per .cpp file.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# External include directories
include_directories(${EXTERNAL}/args)
include_directories(${EXTERNAL}/curl/include)
include_directories(${EXTERNAL}/json/include)
include_directories(${EXTERNAL}/MySQL/include)
include_directories(${EXTERNAL}/StrFormat)
include_directories(${EXTERNAL}/tinylogger)

# External library directories
link_directories(${EXTERNAL}/curl/lib)
link_directories(${EXTERNAL}/MySQL/lib)

add_subdirectory(src)
