cmake_minimum_required(VERSION 3.10)
project(minisql)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Include directories
include_directories(src)

# Source files
set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Storage.cpp
    src/Engine.cpp
    src/HttpServer.cpp
)

# Create executable
add_executable(minisql ${SOURCES})

# Link libraries (none needed for this project)
