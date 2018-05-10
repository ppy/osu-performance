macro(COMMON_INCLUDE_DIRECTORIES)
	include_directories(${SHARED}) # for finding <Shared.h> in-source
	include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}) # for finding <PrecompiledHeader.h> in-source
	include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}) # for the compiler to find PrecompiledHeader.pch

	# necessary, so the proper SDK is included
	if (MSVC)
		include_directories(BEFORE $(WindowsSDK_IncludePath))
		include_directories(BEFORE $(VCInstallDir)include)
	endif()
endmacro(COMMON_INCLUDE_DIRECTORIES)

macro(GENERATE_EXECUTABLE ExecutableName)
	set(PrecompiledHeaderDependency "${ExecutableName}_PrecompiledHeaderDependency")
	set(SharedPrecompiledHeaderDependency "${ExecutableName}_SharedPrecompiledHeaderDependency")

	ADD_PRECOMPILED_HEADER(
		${PrecompiledHeaderDependency} # dependency name
		PrecompiledHeader.h # header file
		. # relative directory
		SOURCES
	)

	if (MSVC)
		ADD_PRECOMPILED_HEADER(
			${SharedPrecompiledHeaderDependency} # dependency name
			Shared.h # header file
			../Shared # relative directory
			SHARED_SOURCES
		)
	endif()
	   
	# Precompiled sources
	ADD_PRECOMPILED_SOURCE(
		PrecompiledHeader.h
		${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeader.cpp
		SOURCES
	)

	add_executable(${ExecutableName} ${SOURCES} ${SHARED_SOURCES} ${HEADERS})
	target_link_libraries(${ExecutableName} ${LIBRARIES})

	if (MSVC)
		set_target_properties(${ExecutableName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN})
		set_target_properties(${ExecutableName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BIN})
		set_target_properties(${ExecutableName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN})
		set_target_properties(${ExecutableName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN})
	elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		set_target_properties(${ExecutableName} PROPERTIES COMPILE_FLAGS "-include PrecompiledHeader.h")
		add_dependencies(${ExecutableName} ${PrecompiledHeaderDependency})
	endif()
endmacro(GENERATE_EXECUTABLE)
