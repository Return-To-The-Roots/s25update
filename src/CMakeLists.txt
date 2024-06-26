# Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(CURL REQUIRED)
find_package(BZip2 1.0.6 REQUIRED)
find_package(Boost 1.55 REQUIRED COMPONENTS filesystem)

set(_sources s25update.cpp md5sum.cpp s25update.h md5sum.h)
if(ClangFormat_FOUND)
    add_ClangFormat_files(${_sources} ../win32/resource.h)
endif()

rttr_set_output_dir(RUNTIME ${RTTR_EXTRA_BINDIR})

add_executable(s25update ${_sources})

if (TARGET CURL::libcurl AND CURL_CONFIG)
    target_link_libraries(s25update PRIVATE CURL::libcurl)
    message(STATUS "Found CURL via CMake config")
else()
    # Heuristic if CURL is a static library and we need to add the define and some libs
    if(CURL_LIBRARIES MATCHES "\.a$" OR CURL_LIBRARIES MATCHES "curl_a" OR (WIN32 AND NOT CURL_LIBRARIES MATCHES "_imp\.lib"))
        message(STATUS "Found static CURL: ${CURL_LIBRARIES}")
        target_compile_definitions(s25update PRIVATE CURL_STATICLIB)
        if(MSVC)
            list(APPEND CURL_LIBRARIES "normaliz.lib;ws2_32.lib;wldap32.lib")
        endif()
    else()
        message(STATUS "Found dynamic CURL: ${CURL_LIBRARIES}")
    endif()
    target_include_directories(s25update SYSTEM PRIVATE ${CURL_INCLUDE_DIRS})
    target_link_libraries(s25update PRIVATE ${CURL_LIBRARIES})
endif()

target_include_directories(s25update SYSTEM PRIVATE)
target_link_libraries(s25update PRIVATE s25util::common BZip2::BZip2 Boost::filesystem Boost::nowide Boost::disable_autolinking)
target_compile_features(s25update PRIVATE cxx_std_17)
if(NOT PLATFORM_NAME OR NOT PLATFORM_ARCH)
    message(FATAL_ERROR "PLATFORM_NAME or PLATFORM_ARCH not set")
endif()
target_compile_definitions(s25update PRIVATE "TARGET=\"${PLATFORM_NAME}\"" "ARCH=\"${PLATFORM_ARCH}\"")

if(WIN32)
    if(MSVC)
        target_sources(s25update PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../win32/s25update.rc")
        set_property(TARGET s25update APPEND_STRING PROPERTY LINK_FLAGS " /MANIFEST:NO")
    else()
        target_compile_definitions(s25update PRIVATE AFX_TARG_DEU)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/s25update.res.o
            COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR}/../win32/
                -i${CMAKE_CURRENT_SOURCE_DIR}/../win32/s25update.rc
                -o ${CMAKE_CURRENT_BINARY_DIR}/s25update.res.o
        )
        target_sources(s25update PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/s25update.res.o)
    endif()
endif()

install(TARGETS s25update RUNTIME DESTINATION ${RTTR_EXTRA_BINDIR})
