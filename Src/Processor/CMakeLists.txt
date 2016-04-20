cmake_minimum_required(VERSION 2.8)

# Processor
set(SOURCES

	${SOURCES}
	
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp

	
	${CMAKE_CURRENT_SOURCE_DIR}/Beatmap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/CCURL.cpp
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
	${SHARED}/Core/StringUtil.cpp
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
	${CMAKE_CURRENT_SOURCE_DIR}/CCURL.h
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
	${SHARED}/Core/StringUtil.h
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
include_directories(${EXTERNAL}/MySQL/include)
include_directories(${EXTERNAL}/curl/include/curl)
include_directories(${EXTERNAL}/StrFormat)

link_directories(${EXTERNAL}/MySQL/lib)
link_directories(${EXTERNAL}/curl/lib)


# Windows specific libraries
if(WIN32)

	set(LIBRARIES

		${LIBRARIES}
		ws2_32
		libmariadb
		libcurl
	)
	
endif(WIN32)

# Linux specific libraries
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)

	# pthread
	find_package(Threads REQUIRED)
	
	set(LIBRARIES

		${LIBRARIES}
		
		${CMAKE_THREAD_LIBS_INIT}
		libmysqlclient.so
		libcurl.so
	)

endif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)


COMMON_INCLUDE_DIRECTORIES()
GENERATE_EXECUTABLE(Client)
