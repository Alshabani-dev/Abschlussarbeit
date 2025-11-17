cmake_minimum_required(VERSION 3.10)
project(minisql)

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
)

# Create executable
add_executable(minisql ${SOURCES})

# Link libraries (pthread for HTTP server)
find_package(Threads REQUIRED)
target_link_libraries(minisql Threads::Threads)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(minisql PRIVATE -Wall -Wextra -pedantic)
endif()
