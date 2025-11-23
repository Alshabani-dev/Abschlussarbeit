cmake_minimum_required(VERSION 3.10)
project(minisql)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Storage.cpp
    src/Engine.cpp
    src/HttpServer.cpp
)

# Add executable
add_executable(minisql ${SOURCES})

# Include directories
target_include_directories(minisql PRIVATE src)

# Link libraries (none needed for this project, everything is standard C++)
