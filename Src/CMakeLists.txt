cmake_minimum_required(VERSION 2.8)

include(${SRC}/PrecompiledHeader.cmake)

# contains common macros for the sub-projects. That is
#  - executable generation
#  - common include patterns
include(${SRC}/Common.cmake)

# Tells the compilers to put the binaries in the BIN folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN})
set(CMAKE_BUILD_TYPE Release)

message(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

add_subdirectory(Processor)
