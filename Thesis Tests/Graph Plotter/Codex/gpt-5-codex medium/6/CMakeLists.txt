cmake_minimum_required(VERSION 3.10)
project(GraphPlotter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_FILES
    src/main.cpp
    src/HttpServer.cpp
    src/PlotRenderer.cpp
    src/Image.cpp
    src/Utils.cpp
)

add_executable(graph_server ${SRC_FILES})

target_include_directories(graph_server PRIVATE src)
