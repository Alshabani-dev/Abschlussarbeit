cmake_minimum_required(VERSION 3.10)
project(MinimalSQLInterpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Executable
add_executable(minisql
    src/main.cpp
    src/Engine.cpp
    src/Storage.cpp
    src/Utils.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/HttpServer.cpp
)

# Include directories
target_include_directories(minisql PRIVATE src)

# Create data directory if it doesn't exist
add_custom_command(
    TARGET minisql PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/data
)
