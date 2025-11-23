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

# Platform-specific libraries
if(UNIX)
    target_link_libraries(minisql pthread)
endif()

# Enable warnings
if(MSVC)
    target_compile_options(minisql PRIVATE /W4)
else()
    target_compile_options(minisql PRIVATE -Wall -Wextra -pedantic)
endif()
