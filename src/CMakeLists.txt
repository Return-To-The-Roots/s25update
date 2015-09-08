
FIND_PACKAGE(CURL REQUIRED)
FIND_PACKAGE(BZip2 REQUIRED)
FIND_PACKAGE(Boost 1.55 COMPONENTS system filesystem REQUIRED)

INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR}
                    ${BZIP2_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR}
                    )
ADD_DEFINITIONS(${BZIP2_DEFINITIONS})

# bzip linkerbug-fix
SET(SOURCES_BZIP "")
IF ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
	IF( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3" )
		SET(SOURCES_BZIP 
			${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/blocksort.c
		    ${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/huffman.c 
		    ${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/crctable.c
		    ${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/randtable.c 
		    ${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/compress.c
		    ${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/decompress.c 
		    ${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3/bzlib.c
		   )
	ENDIF( IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/bzip2-1.0.3" )
ENDIF ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )

################################################################################

SET(s25update_SRCS main.cpp md5.cpp md5sum.cpp)

IF ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" AND NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	ADD_DEFINITIONS("-DAFX_TARG_DEU")
	ADD_DEFINITIONS("-D_WIN32_WCE")

	ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/s25update.res.o COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR}/../win32/ -i${CMAKE_CURRENT_SOURCE_DIR}/../win32/s25update.rc -o ${CMAKE_CURRENT_BINARY_DIR}/s25update.res.o)

	SET(s25update_SRCS ${s25update_SRCS} s25update.res.o)
ENDIF ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" AND NOT ${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")

IF (MSVC)
	SET(s25update_RC "${CMAKE_CURRENT_SOURCE_DIR}/../win32/s25update.rc")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
ELSE()
	SET(s25update_RC "")
ENDIF()

ADD_EXECUTABLE(s25update ${s25update_SRCS} ${SOURCES_BZIP} ${s25update_RC})

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_SYSTEM_NAME)
ADD_DEFINITIONS("-DTARGET=\"${LOWER_SYSTEM_NAME}\"")
ADD_DEFINITIONS("-DARCH=\"${PLATFORM_ARCH}\"")

TARGET_LINK_LIBRARIES(s25update ${CURL_LIBRARIES} ${BZIP2_LIBRARIES} ${Boost_LIBRARIES})

IF ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" AND NOT MSVC)
	ADD_CUSTOM_COMMAND(TARGET s25update POST_BUILD COMMAND if [ -f s25update.exe ] \; then cp -v s25update.exe s25update \; fi)
ENDIF ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" AND NOT MSVC)

IF (NOT MSVC)
	ADD_CUSTOM_COMMAND(TARGET s25update POST_BUILD COMMAND cp -v s25update ../../RTTR/)
ENDIF()

INSTALL(TARGETS s25update RUNTIME DESTINATION ${DATADIR}/RTTR/)

################################################################################
