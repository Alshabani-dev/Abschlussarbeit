cmake_minimum_required(VERSION 3.10)
project(minisql CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(minisql
    src/main.cpp
    src/Engine.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Storage.cpp
    src/HttpServer.cpp)

target_include_directories(minisql PRIVATE src)

target_compile_options(minisql PRIVATE -Wall -Wextra -pedantic)
