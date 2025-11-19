cmake_minimum_required(VERSION 3.10)
project(GraphPlotter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(graphserver
    src/main.cpp
    src/HttpServer.cpp
    src/PlotRenderer.cpp
    src/Image.cpp
    src/Utils.cpp
)

target_include_directories(graphserver PRIVATE src)

if (MSVC)
    target_compile_options(graphserver PRIVATE /W4)
else()
    target_compile_options(graphserver PRIVATE -Wall -Wextra -pedantic -Werror=return-type)
endif()
