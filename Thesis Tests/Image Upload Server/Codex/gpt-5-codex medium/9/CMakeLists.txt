cmake_minimum_required(VERSION 3.16)

project(SimpleCppWebServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(webserver
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/Server.cpp
    ${SRC_DIR}/HttpRequest.cpp
    ${SRC_DIR}/HttpResponse.cpp
    ${SRC_DIR}/FileHandler.cpp
    ${SRC_DIR}/UploadHandler.cpp
)

target_include_directories(webserver PRIVATE ${SRC_DIR})

target_compile_options(webserver PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
)

if (UNIX)
    target_link_options(webserver PRIVATE "-Wl,-rpath,/lib/x86_64-linux-gnu")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(webserver PRIVATE stdc++fs)
endif()
