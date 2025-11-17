cmake_minimum_required(VERSION 3.10)
project(MinimalSQLInterpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Engine.cpp
    src/Storage.cpp
    src/HttpServer.cpp
    src/Utils.cpp
)

# Headers
set(HEADERS
    src/Lexer.h
    src/Parser.h
    src/Ast.h
    src/Engine.h
    src/Storage.h
    src/HttpServer.h
    src/Utils.h
)

# Create executable
add_executable(minisql ${SOURCES} ${HEADERS})

# Enable warnings
if(MSVC)
    target_compile_options(minisql PRIVATE /W4)
else()
    target_compile_options(minisql PRIVATE -Wall -Wextra -Wpedantic)
endif()
