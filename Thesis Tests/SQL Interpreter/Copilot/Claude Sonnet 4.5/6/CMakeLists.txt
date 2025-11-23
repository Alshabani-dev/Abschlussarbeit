cmake_minimum_required(VERSION 3.10)
project(MiniSQL VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Storage.cpp
    src/Engine.cpp
    src/HttpServer.cpp
)

# Header files (for IDE organization)
set(HEADERS
    src/Utils.h
    src/Ast.h
    src/Lexer.h
    src/Parser.h
    src/Storage.h
    src/Engine.h
    src/HttpServer.h
)

# Create executable
add_executable(minisql ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(minisql PRIVATE src)

# Installation
install(TARGETS minisql DESTINATION bin)

# Print build information
message(STATUS "MiniSQL Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
