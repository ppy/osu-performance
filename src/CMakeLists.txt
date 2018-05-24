cmake_minimum_required(VERSION 2.8)

# Processor
set(SOURCES
	performance/main.cpp

	performance/Beatmap.cpp
	performance/CURL.cpp
	performance/DDog.cpp
	performance/Processor.cpp
	performance/Score.cpp
	performance/User.cpp
	performance/UUID.cpp

	performance/osu/StandardScore.cpp
	performance/taiko/TaikoScore.cpp
	performance/catch/CatchTheBeatScore.cpp
	performance/mania/ManiaScore.cpp

	shared/Active.cpp
	shared/Threading.cpp

	shared/DatabaseConnection.cpp
	shared/QueryResult.cpp
	shared/UpdateBatch.cpp
)

if (WIN32)
	set(LIBRARIES ws2_32)

	# Link against different pre-built binaries if 64-bit
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(LIBRARIES ${LIBRARIES} libmariadb libcurl)
	else()
		set(LIBRARIES ${LIBRARIES} libmariadb32 libcurl32)
	endif()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	find_package(Threads REQUIRED)
	set(LIBRARIES ${CMAKE_THREAD_LIBS_INIT} mysqlclient curl)
endif()

add_executable(osu-performance ${SOURCES})
target_link_libraries(osu-performance ${LIBRARIES})

if (MSVC)
	set_target_properties(${ExecutableName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN})
	set_target_properties(${ExecutableName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BIN})
	set_target_properties(${ExecutableName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN})
	set_target_properties(${ExecutableName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN})
endif()

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
