cmake_minimum_required(VERSION 3.13)
project(webserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

set(SOURCES
    src/main.cpp
    src/Server.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/FileHandler.cpp
    src/UploadHandler.cpp
)

add_executable(webserver ${SOURCES})

target_include_directories(webserver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
