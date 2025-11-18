cmake_minimum_required(VERSION 3.12)
project(minisql LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(minisql
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Engine.cpp
    src/Storage.cpp
    src/HttpServer.cpp
)

target_include_directories(minisql PRIVATE src)
