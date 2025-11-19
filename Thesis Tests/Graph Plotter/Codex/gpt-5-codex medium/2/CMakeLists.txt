cmake_minimum_required(VERSION 3.10)
project(GraphServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(graph_server
    src/main.cpp
    src/Utils.cpp
    src/Image.cpp
    src/PlotRenderer.cpp
    src/HttpServer.cpp
)

target_include_directories(graph_server PRIVATE src)

target_compile_options(graph_server PRIVATE -Wall -Wextra -pedantic -Werror=return-type)
