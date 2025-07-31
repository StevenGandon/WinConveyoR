cmake_minimum_required(VERSION 3.15)
project(libwconr LANGUAGES C)

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

include(FetchContent)

FetchContent_Declare(
  curl
  URL https://curl.se/download/curl-7.86.0.tar.gz
)
FetchContent_MakeAvailable(curl)

set(LIBWCONR_SOURCES
    src/download_package.c
    src/http_headers.c
    src/http_parsing.c
    src/http_request.c
    src/http_response.c
    src/wcr_state.c

    src/modules/source/source_handler.c
)

if(WIN32)
    list(APPEND LIBWCONR_SOURCES src/win/http.c)
else()
    list(APPEND LIBWCONR_SOURCES src/posix/http.c)
endif()

add_library(wconr SHARED ${LIBWCONR_SOURCES})

target_link_libraries(wconr PRIVATE CURL::libcurl)
if(WIN32)
    target_link_libraries(wconr PRIVATE ws2_32)
endif()

target_include_directories(wconr
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

message(STATUS "Library output directory: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "Runtime output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Archive output directory: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

set_target_properties(wconr PROPERTIES
    PREFIX ""
    OUTPUT_NAME "libwconr"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

add_custom_command(TARGET wconr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:libcurl>
        $<TARGET_FILE_DIR:wconr>
    COMMAND ${CMAKE_COMMAND} -E echo "libcurl copied to: $<TARGET_FILE_DIR:wconr>"
    COMMENT "Copying libcurl shared library to output directory and printing the path"
)
