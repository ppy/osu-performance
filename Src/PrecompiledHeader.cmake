macro(ADD_PRECOMPILED_HEADER Name PrecompiledHeader RelPath SourcesVar)


	if(MSVC)
		SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pch")
		SET(SourcesDereferenced ${${SourcesVar}})

		SET_SOURCE_FILES_PROPERTIES(
			${SourcesDereferenced}
			PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledBinary}\" /Fp\"${PrecompiledBinary}\""
			OBJECT_DEPENDS "${PrecompiledBinary}")  

	endif(MSVC)


	if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
 
		SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledHeader}.gch")

		# Get include flags
		get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
		set(IncludeFlags)
		foreach(dir ${dirs})
			set(IncludeFlags ${IncludeFlags} "-I${dir}")
		endforeach()
		string(REPLACE " " ";" IncludeFlags "${IncludeFlags}")


		# Get definition flags (like __CLIENT, __WORLDSERVER etc.)
		get_property(defs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
		set(DefFlags)
		foreach(def ${defs})
			set(DefFlags ${DefFlags} "-D${def}")
		endforeach()
		string(REPLACE " " ";" DefFlags "${DefFlags}")


		# Get compiler flags in a parseable formatr
		if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
			set(CompilerFlags ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE})
		elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
			set(CompilerFlags ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG})
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
			IMPLICIT_DEPENDS CXX "${CMAKE_CURRENT_SOURCE_DIR}/${RelPath}/${PrecompiledHeader}")

		add_custom_target(${Name} DEPENDS ${PrecompiledBinary})
		
	endif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)

endmacro(ADD_PRECOMPILED_HEADER)


macro(ADD_PRECOMPILED_SOURCE PrecompiledHeader PrecompiledSource SourcesVar)


	IF(MSVC)
		SET(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pch")
		SET_SOURCE_FILES_PROPERTIES(
			${PrecompiledSource}
			PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
			OBJECT_OUTPUTS "${PrecompiledBinary}")
											   
		# Add precompiled header to SourcesVar
		LIST(APPEND ${SourcesVar} ${PrecompiledSource})
	ENDIF(MSVC)
	
endmacro(ADD_PRECOMPILED_SOURCE)
