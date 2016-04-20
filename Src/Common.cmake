macro(COMMON_INCLUDE_DIRECTORIES)

	include_directories(${SHARED}) # for finding <Shared.h> in-source
	include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}) # for finding <PrecompiledHeader.h> in-source
	include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}) # for the compiler to find PrecompiledHeader.pch

	# necessary, so the proper SDK is included
	if(MSVC)
		include_directories(BEFORE $(WindowsSDK_IncludePath))
		include_directories(BEFORE $(VCInstallDir)include)
	endif(MSVC)
	
endmacro(COMMON_INCLUDE_DIRECTORIES)


macro(GENERATE_EXECUTABLE BaseName)


	set(PrecompiledHeaderDependency "${BaseName}_PrecompiledHeaderDependency")
	set(SharedPrecompiledHeaderDependency "${BaseName}_SharedPrecompiledHeaderDependency")

	ADD_PRECOMPILED_HEADER(
		${PrecompiledHeaderDependency} # dependency name
		PrecompiledHeader.h # header file
		. # relative directory
		SOURCES)

	if(MSVC)
		ADD_PRECOMPILED_HEADER(
			${SharedPrecompiledHeaderDependency} # dependency name
			Shared.h # header file
			../Shared # relative directory
			SHARED_SOURCES)
	endif(MSVC)
	   
	# Precompiled sources
	ADD_PRECOMPILED_SOURCE(
		PrecompiledHeader.h
		${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeader.cpp
		SOURCES)


	if(WIN32)
		set(Executable "${BaseName}_win32")
	elseif(APPLE)
		set(Executable "${BaseName}_mac")
	else()
		set(Executable "${BaseName}_linux")
	endif()



	message(STATUS "${BaseName} executable: ${Executable}")


	add_executable(${Executable} ${SOURCES} ${SHARED_SOURCES} ${HEADERS})
	set_target_properties(${Executable} PROPERTIES DEBUG_POSTFIX "_d")
	target_link_libraries(${Executable} ${LIBRARIES})

	if(MSVC)

		set_target_properties(${Executable} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN})
		set_target_properties(${Executable} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN})
		
	elseif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)

		set_target_properties(${Executable} PROPERTIES COMPILE_FLAGS "-include PrecompiledHeader.h")
		add_dependencies(${Executable} ${PrecompiledHeaderDependency})
		
	endif()


endmacro(GENERATE_EXECUTABLE)
