cmake_minimum_required(VERSION 3.10)
project(MinimalSQLInterpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Storage.cpp
    src/Engine.cpp
    src/HttpServer.cpp
    src/Utils.cpp
)

# Executable
add_executable(minisql ${SOURCES})

# Include directories
target_include_directories(minisql PRIVATE src)

# Enable warnings
target_compile_options(minisql PRIVATE -Wall -Wextra -pedantic)

# Link libraries
target_link_libraries(minisql PRIVATE pthread)
