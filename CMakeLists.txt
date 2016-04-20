cmake_minimum_required(VERSION 2.8)



project(ppv2Processor)


# Specify custom module search path
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/" ${CMAKE_MODULE_PATH})


# If we default to gcc we better switch to clang
if(UNIX)

	if(APPLE)
		set(CMAKE_C_COMPILER clang-3.5)
		set(CMAKE_CXX_COMPILER clang++-3.5)
	else(APPLE)
		set(CMAKE_C_COMPILER clang)
		set(CMAKE_CXX_COMPILER clang++)
		#set(CMAKE_C_COMPILER gcc-4.8)
		#set(CMAKE_CXX_COMPILER g++-4.8)
	endif(APPLE)
	
	set(CMAKE_COMPILER_IS_CLANGXX 1)
endif(UNIX)
	


message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")


# Set variable for shared folder and for libraries / include folder
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/Src)
set(SHARED ${SRC}/Shared)
set(EXTERNAL ${SRC}/External)
set(BIN ${CMAKE_CURRENT_SOURCE_DIR}/Bin)




if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)

	if(APPLE)
		set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ -I/opt/X11/include -I/usr/local/lib/llvm-3.3/c++/v1")
		link_directories(/usr/local/lib/llvm-3.3/usr/lib)
	else(APPLE)
		set(CMAKE_CXX_FLAGS "-std=c++11")
	endif(APPLE)
	
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


