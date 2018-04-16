macro(ADD_PRECOMPILED_HEADER Name PrecompiledHeader RelPath SourcesVar)
	if (MSVC)
		set(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pch")
		set(SourcesDereferenced ${${SourcesVar}})

		set_source_files_properties(
			${SourcesDereferenced}
			PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledBinary}\" /Fp\"${PrecompiledBinary}\""
			OBJECT_DEPENDS "${PrecompiledBinary}"
		)
	endif()

	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		set(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledHeader}.gch")

		# Get include flags
		get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
		set(IncludeFlags)

		foreach (dir ${dirs})
			set(IncludeFlags ${IncludeFlags} "-I${dir}")
		endforeach()

		string(REPLACE " " ";" IncludeFlags "${IncludeFlags}")

		get_property(defs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
		set(DefFlags)

		foreach (def ${defs})
			set(DefFlags ${DefFlags} "-D${def}")
		endforeach()
		
		string(REPLACE " " ";" DefFlags "${DefFlags}")

		# Get compiler flags in a parseable formatr
		if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
			set(CompilerFlags ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE})
		elseif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
			set(CompilerFlags ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG})
		elseif (${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
			set(CompilerFlags ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
		elseif (${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
			set(CompilerFlags ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_MINSIZEREL})
		else()
			set(CompilerFlags ${CMAKE_CXX_FLAGS})
		endif()

		string(REPLACE " " ";" CompilerFlags "${CompilerFlags}")

		add_custom_command(
			OUTPUT ${PrecompiledBinary}
			COMMAND ${CMAKE_CXX_COMPILER} ${DefFlags} ${CompilerFlags} ${IncludeFlags}
			-x c++-header "${CMAKE_CURRENT_SOURCE_DIR}/${RelPath}/${PrecompiledHeader}"
			-o "${PrecompiledBinary}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${RelPath}/${PrecompiledHeader}"
			IMPLICIT_DEPENDS CXX "${CMAKE_CURRENT_SOURCE_DIR}/${RelPath}/${PrecompiledHeader}"
		)

		add_custom_target(${Name} DEPENDS ${PrecompiledBinary})
	endif()
endmacro(ADD_PRECOMPILED_HEADER)

macro(ADD_PRECOMPILED_SOURCE PrecompiledHeader PrecompiledSource SourcesVar)
	if (MSVC)
		set(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pch")
		set_source_files_properties(
			${PrecompiledSource}
			PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
			OBJECT_OUTPUTS "${PrecompiledBinary}"
		)

		# Add precompiled header to SourcesVar
		list(APPEND ${SourcesVar} ${PrecompiledSource})
	endif()
endmacro(ADD_PRECOMPILED_SOURCE)
