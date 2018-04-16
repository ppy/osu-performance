cmake_minimum_required(VERSION 2.8)

project(osu-performance)

# Set variable for shared folder and for libraries / include folder
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/Src)
set(SHARED ${SRC}/Shared)
set(EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies)
set(BIN ${CMAKE_CURRENT_SOURCE_DIR}/Bin)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
endif()

add_subdirectory(Src)
