cmake_minimum_required(VERSION 3.10)
project(GraphServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/main.cpp
    src/HttpServer.cpp
    src/PlotRenderer.cpp
    src/Image.cpp
    src/Utils.cpp
)

add_executable(graphserver ${SOURCES})

target_include_directories(graphserver PRIVATE src)

target_compile_options(graphserver PRIVATE -Wall -Wextra -Wpedantic)
