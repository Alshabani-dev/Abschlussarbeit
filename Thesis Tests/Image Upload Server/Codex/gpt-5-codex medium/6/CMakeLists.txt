cmake_minimum_required(VERSION 3.15)
project(webserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(webserver
    src/main.cpp
    src/Server.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/FileHandler.cpp
    src/UploadHandler.cpp
)

target_include_directories(webserver PRIVATE src)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(webserver PRIVATE -Wall -Wextra -pedantic -Werror=return-type)
endif()
