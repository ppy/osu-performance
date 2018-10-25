cmake_minimum_required(VERSION 2.8)

# Processor
set(SOURCES
	Common.cpp ../include/pp/Common.h

	performance/main.cpp

	performance/Beatmap.cpp ../include/pp/performance/Beatmap.h
	performance/CURL.cpp ../include/pp/performance/CURL.h
	performance/DDog.cpp ../include/pp/performance/DDog.h
	performance/Processor.cpp ../include/pp/performance/Processor.h
	performance/Score.cpp ../include/pp/performance/Score.h
	performance/User.cpp ../include/pp/performance/User.h
	performance/UUID.cpp ../include/pp/performance/UUID.h

	performance/osu/OsuScore.cpp ../include/pp/performance/osu/OsuScore.h
	performance/taiko/TaikoScore.cpp ../include/pp/performance/taiko/TaikoScore.h
	performance/catch/CatchScore.cpp ../include/pp/performance/catch/CatchScore.h
	performance/mania/ManiaScore.cpp ../include/pp/performance/mania/ManiaScore.h

	shared/Active.cpp ../include/pp/shared/Active.h
	shared/Threading.cpp ../include/pp/shared/Threading.h
	shared/DatabaseConnection.cpp ../include/pp/shared/DatabaseConnection.h
	shared/QueryResult.cpp ../include/pp/shared/QueryResult.h
	shared/UpdateBatch.cpp ../include/pp/shared/UpdateBatch.h
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
	set_target_properties(osu-performance PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN})
	set_target_properties(osu-performance PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BIN})
	set_target_properties(osu-performance PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN})
	set_target_properties(osu-performance PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN})
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
