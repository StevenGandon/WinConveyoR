cmake_minimum_required(VERSION 3.15)
project(libwconr VERSION 1.0.0 LANGUAGES C)

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

set_target_properties(wconr PROPERTIES
    OUTPUT_NAME "libwconr"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install(TARGETS wconr
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(
    DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# That’s it! Now you can build:
#   mkdir build && cd build
#   cmake -G "Visual Studio 17 2022" -A x64 ..  (for Windows)
#   cmake --build . --config Release
#   cmake --install . --config Release
