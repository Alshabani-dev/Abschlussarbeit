cmake_minimum_required(VERSION 3.10)
project(minisql)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(minisql
    src/main.cpp
    src/Engine.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Storage.cpp
    src/HttpServer.cpp
)

target_include_directories(minisql PRIVATE src)
