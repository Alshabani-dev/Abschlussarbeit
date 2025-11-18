cmake_minimum_required(VERSION 3.15)
project(minisql LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(minisql
    src/main.cpp
    src/Utils.h
    src/Lexer.cpp
    src/Lexer.h
    src/Parser.cpp
    src/Parser.h
    src/Ast.h
    src/Storage.cpp
    src/Storage.h
    src/Engine.cpp
    src/Engine.h
    src/HttpServer.cpp
    src/HttpServer.h
)

target_include_directories(minisql PRIVATE src)
