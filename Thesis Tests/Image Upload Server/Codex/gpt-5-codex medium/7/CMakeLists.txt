cmake_minimum_required(VERSION 3.12)
project(SimpleWebServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB WEB_SERVER_SOURCES
    ${SRC_DIR}/*.cpp
)

add_executable(webserver ${WEB_SERVER_SOURCES})

target_include_directories(webserver PRIVATE ${SRC_DIR})

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(webserver PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()
