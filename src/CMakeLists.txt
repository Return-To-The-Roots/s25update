FIND_PACKAGE(CURL REQUIRED)
FIND_PACKAGE(BZip2 REQUIRED)
FIND_PACKAGE(Boost 1.55 REQUIRED COMPONENTS system filesystem)

INCLUDE_DIRECTORIES(SYSTEM
                    ${CURL_INCLUDE_DIR}
                    ${BZIP2_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR}
                    )

################################################################################

SET(s25update_SRCS s25update.cpp md5.cpp md5sum.cpp)
SET(s25update_HEADERS s25update.h md5.h md5sum.h)
source_group("Source Files" FILES ${s25update_SRCS})
source_group("Header Files" FILES ${s25update_HEADERS})
if(ClangFormat_FOUND)
    add_ClangFormat_files(${s25update_SRCS} ${s25update_HEADERS})
endif()

IF (WIN32 AND NOT CMAKE_HOST_WIN32)
	ADD_DEFINITIONS("-DAFX_TARG_DEU")
	ADD_DEFINITIONS("-D_WIN32_WCE")

	ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/s25update.res.o COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR}/../win32/ -i${CMAKE_CURRENT_SOURCE_DIR}/../win32/s25update.rc -o ${CMAKE_CURRENT_BINARY_DIR}/s25update.res.o)

	SET(s25update_SRCS ${s25update_SRCS} s25update.res.o)
ENDIF()

IF (MSVC)
	SET(s25update_RC "${CMAKE_CURRENT_SOURCE_DIR}/../win32/s25update.rc")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
ELSE()
	SET(s25update_RC "")
ENDIF()

STRING(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_SYSTEM_NAME)
ADD_DEFINITIONS("-DTARGET=\"${LOWER_SYSTEM_NAME}\"")
ADD_DEFINITIONS("-DARCH=\"${PLATFORM_ARCH}\"")

ADD_EXECUTABLE(s25update ${s25update_SRCS} ${SOURCES_BZIP} ${s25update_RC})
TARGET_LINK_LIBRARIES(s25update ${CURL_LIBRARIES} ${BZIP2_LIBRARIES} ${Boost_LIBRARIES} nowide-static)

rttr_set_output_dir(RUNTIME ${RTTR_BINDIR}/RTTR)

INSTALL(TARGETS s25update RUNTIME DESTINATION ${RTTR_BINDIR}/RTTR)
