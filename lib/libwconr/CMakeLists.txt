cmake_minimum_required(VERSION 3.15)

project(libwconr
    LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# ========= Options =========
option(SANITIZE     "Build with AddressSanitizer + UBSan" OFF)
option(BUILD_TESTS  "Build test executable" OFF)

if(BUILD_TESTS)
    enable_testing()
endif()

if(NOT DEFINED DIST_DIR)
    get_filename_component(CWD_NAME ${CMAKE_SOURCE_DIR} NAME)
    set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist/${CWD_NAME}")
else()
    get_filename_component(DIST_DIR "${DIST_DIR}" ABSOLUTE BASE_DIR "${CMAKE_BINARY_DIR}")
endif()

if(NOT DEFINED TARGET_ARCH)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(TARGET_ARCH "i386")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(TARGET_ARCH "amd64")
    endif()
endif()

# ========= curl =========

include(FetchContent)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "Disable curl executable" FORCE)
set(CURL_STATICLIB OFF CACHE BOOL "Build curl as shared" FORCE)

if(WIN32)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "Use native Schannel" FORCE)
elseif(APPLE)
    set(CURL_USE_SECURETRANSPORT ON CACHE BOOL "Use SecureTransport" FORCE)
else()
    set(CURL_USE_OPENSSL ON CACHE BOOL "Use OpenSSL" FORCE)
endif()

FetchContent_Declare(
  curl
  URL https://curl.se/download/curl-8.11.1.tar.gz
)
FetchContent_MakeAvailable(curl)

set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_AND_STATIC_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_CUSTOM_COMPILER_FLAGS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  cjson
  GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
  GIT_TAG v1.7.18
)
FetchContent_MakeAvailable(cjson)

set(LIBXML2_WITH_PYTHON OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_LZMA OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_ICONV OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    libxml2
    GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git
    GIT_TAG v2.14.5
)

FetchContent_MakeAvailable(libxml2)

find_package(OpenSSL REQUIRED)

#

if(SANITIZE)
    message(STATUS "[WinConveryoR] Sanitizers ON: ASan + UBSan")
    add_compile_options(
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-common
        -fstack-protector-all
        -fstack-check
        -g
    )
    add_link_options(-fsanitize=address,undefined)
endif()

# ===== Tests =====
if(BUILD_TESTS)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/units/libwconr
        ${CMAKE_CURRENT_BINARY_DIR}/tests/units/libwconr
    )
endif()

# ===== dist deploy =====
add_custom_target(dist ALL)

add_dependencies(dist wconr)

set(LIBWCONR_SOURCES
    src/wcr_event.c
    src/download_package.c
    src/pkg_downloader.c
    src/file_utils.c
    src/pkg_parsing.c
    src/sync_package_list.c
    src/install_package.c
    src/uninstall_package.c
    src/pkg_registry.c
    src/http_headers.c
    src/http_parsing.c
    src/http_request.c
    src/http_response.c
    src/wcr_state.c
    src/wcr_client.c
    src/wcr_crypt.c
    src/pkg_specifier.c

    src/modules/source/source_handler.c

    src/pars_wizard.c
    src/wizard_endian.c
    src/wizard_section.c
    src/vm.c
    src/vm_parse.c
    src/vm_decode.c
    src/vm_handlers.c
)

if(WIN32)
    list(APPEND LIBWCONR_SOURCES src/win/http.c)
else()
    list(APPEND LIBWCONR_SOURCES src/posix/http.c)
endif()

add_library(wconr SHARED ${LIBWCONR_SOURCES})

target_compile_options(wconr
    PRIVATE
        -Wall -Wextra -Werror
        $<IF:$<BOOL:${SANITIZE}>,-O1,-O2>
)

target_link_libraries(wconr
    PRIVATE CURL::libcurl
    LibXml2::LibXml2
    OpenSSL::Crypto
    cjson
)

if(WIN32)
    target_link_libraries(wconr PRIVATE ws2_32)
else()
    target_link_libraries(wconr PRIVATE pthread)
endif()

target_include_directories(wconr
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${cjson_SOURCE_DIR}
)

set_target_properties(wconr PROPERTIES
    PREFIX ""
    OUTPUT_NAME "libwconr"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# libs
foreach(lib libcurl cjson wconr)
    add_custom_command(TARGET dist POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/lib"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${lib}>"
                "${DIST_DIR}/lib/"
        )
endforeach()

#libxml2
foreach(lib LibXml2)
    if(TARGET ${lib})
        get_target_property(lib_type ${lib} TYPE)
        if(lib_type STREQUAL "SHARED_LIBRARY")
            add_custom_command(TARGET dist POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/lib"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${lib}>
                    "${DIST_DIR}/lib/"
            )
        endif()
    endif()
endforeach()

# includes
foreach(lib libwconr)
    add_custom_command(TARGET dist POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DIST_DIR}/include"
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/include/libwconr.h"
                "${DIST_DIR}/include/"
    )
endforeach()

if(UNIX AND NOT APPLE)
    find_program(PATCHELF_EXECUTABLE patchelf)
    if(PATCHELF_EXECUTABLE)
        add_custom_command(TARGET dist POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                -DLIB_DIR=${DIST_DIR}/lib
                -DBIN_DIR=${DIST_DIR}/bin
                -P ${CMAKE_SOURCE_DIR}/../../toolchain/strip_soversion.cmake
        )
    else()
        message(WARNING "patchelf not found versioned .so files will be kept")
    endif()
endif()
