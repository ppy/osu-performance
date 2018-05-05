cmake_minimum_required(VERSION 2.8)

# Processor
set(SOURCES
	${SOURCES}

	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/Beatmap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/CURL.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DDog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Processor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Score.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/User.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UUID.cpp

	# standard
	${CMAKE_CURRENT_SOURCE_DIR}/Standard/StandardScore.cpp

	# ctb
	${CMAKE_CURRENT_SOURCE_DIR}/CatchTheBeat/CatchTheBeatScore.cpp

	# taiko
	${CMAKE_CURRENT_SOURCE_DIR}/Taiko/TaikoScore.cpp

	# mania
	${CMAKE_CURRENT_SOURCE_DIR}/Mania/ManiaScore.cpp
)

set(SHARED_SOURCES
	${SHARED_SOURCES}

	${SHARED}/Config.cpp

	${SHARED}/Core/Active.cpp
	${SHARED}/Core/Exception.cpp
	${SHARED}/Core/Logger.cpp
	${SHARED}/Core/Threading.cpp

	${SHARED}/Network/DatabaseConnection.cpp
	${SHARED}/Network/QueryResult.cpp
	${SHARED}/Network/UpdateBatch.cpp
)

set(HEADERS
	${HEADERS}

	# precompiled header
	${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeader.h

	${CMAKE_CURRENT_SOURCE_DIR}/Beatmap.h
	${CMAKE_CURRENT_SOURCE_DIR}/CURL.h
	${CMAKE_CURRENT_SOURCE_DIR}/DDog.h
	${CMAKE_CURRENT_SOURCE_DIR}/Processor.h
	${CMAKE_CURRENT_SOURCE_DIR}/Score.h
	${CMAKE_CURRENT_SOURCE_DIR}/SharedEnums.h
	${CMAKE_CURRENT_SOURCE_DIR}/User.h
	${CMAKE_CURRENT_SOURCE_DIR}/UUID.h

	# standard
	${CMAKE_CURRENT_SOURCE_DIR}/Standard/StandardScore.h

	# ctb
	${CMAKE_CURRENT_SOURCE_DIR}/CatchTheBeat/CatchTheBeatScore.h

	# taiko
	${CMAKE_CURRENT_SOURCE_DIR}/Taiko/TaikoScore.h

	# mania
	${CMAKE_CURRENT_SOURCE_DIR}/Mania/ManiaScore.h

	# shared stuff
	${SHARED}/Config.h
	${SHARED}/configVariables.h
	${SHARED}/Shared.h

	${SHARED}/Core/Active.h
	${SHARED}/Core/Exception.h
	${SHARED}/Core/Logger.h
	${SHARED}/Core/SharedQueue.h
	${SHARED}/Core/StringUtils.h
	${SHARED}/Core/Threading.h
	${SHARED}/Core/Types.h

	${SHARED}/Math/Math.h

	${SHARED}/Network/DatabaseConnection.h
	${SHARED}/Network/QueryResult.h
	${SHARED}/Network/UpdateBatch.h
)

# Necessary so all shared source files can have the unified #include <Shared.h> statement.
# MSVC would complain if some use "../Shared" and some "Shared" without having those exact names specified
# per .cpp file.
include_directories(${SHARED})

# Add external include directories
include_directories(${EXTERNAL}/args)
include_directories(${EXTERNAL}/curl/include)
include_directories(${EXTERNAL}/MySQL/include)
include_directories(${EXTERNAL}/StrFormat)

# External library directories
link_directories(${EXTERNAL}/curl/lib)
link_directories(${EXTERNAL}/MySQL/lib)

if (WIN32)
	set(LIBRARIES ${LIBRARIES} ws2_32)

	# Link against different pre-built binaries if 64-bit
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(LIBRARIES ${LIBRARIES} libmariadb libcurl)
	else()
		set(LIBRARIES ${LIBRARIES} libmariadb32 libcurl32)
	endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	find_package(Threads REQUIRED)
	set(LIBRARIES ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} mysqlclient curl)
endif()

COMMON_INCLUDE_DIRECTORIES()
GENERATE_EXECUTABLE(osu-performance)

if (WIN32)
	# Copy DLLs
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		add_custom_command(TARGET osu-performance POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${EXTERNAL}/curl/dll/64bit/libcurl.dll"
				$<TARGET_FILE_DIR:osu-performance>
		)

		add_custom_command(TARGET osu-performance POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${EXTERNAL}/MySQL/dll/64bit/libmariadb.dll"
				$<TARGET_FILE_DIR:osu-performance>
		)
	else()
		add_custom_command(TARGET osu-performance POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${EXTERNAL}/curl/dll/32bit/libcurl.dll"
				$<TARGET_FILE_DIR:osu-performance>
		)

		add_custom_command(TARGET osu-performance POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${EXTERNAL}/MySQL/dll/32bit/libmariadb.dll"
				$<TARGET_FILE_DIR:osu-performance>
		)
	endif()
endif()
