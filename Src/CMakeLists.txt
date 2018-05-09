cmake_minimum_required(VERSION 2.8)

include(${SRC}/PrecompiledHeader.cmake)

# contains common macros for the sub-projects. That is
#  - executable generation
#  - common include patterns
include(${SRC}/Common.cmake)

# Tells the compilers to put the binaries in the BIN folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN})

# Set a default configuration if none was specified
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No release type specified. Setting to 'Release'.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

add_subdirectory(Processor)
