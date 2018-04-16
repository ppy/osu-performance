cmake_minimum_required(VERSION 2.8)

project(ppv2Processor)

# Specify custom module search path
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/" ${CMAKE_MODULE_PATH})

message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

# Set variable for shared folder and for libraries / include folder
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/Src)
set(SHARED ${SRC}/Shared)
set(EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies)
set(BIN ${CMAKE_CURRENT_SOURCE_DIR}/Bin)

if(UNIX)
	set(CMAKE_CXX_FLAGS "-std=c++11")
	set(CMAKE_CXX_FLAGS_RELEASE "-g -O3")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
elseif(MSVC)
	set(CMAKE_C_FLAGS_DEBUG "/D_DEBUG /MDd /Zi  /Ob0 /Od /RTC1")
	set(CMAKE_C_FLAGS_MINSIZEREL "/MD /O1 /Ob1 /D NDEBUG")
	set(CMAKE_C_FLAGS_RELEASE "/GL /MD /Zi /O2 /Ob2 /D NDEBUG")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "/GL /MD /Zi /O2 /Ob1 /D NDEBUG")
	
	set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "/MD /O1 /Ob1 /D NDEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "/MD /Zi /O2 /Ob2 /D NDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /Zi /O2 /Ob1 /D NDEBUG")

	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG /SAFESEH:NO")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /SAFESEH:NO")
endif()

add_subdirectory(Src)
