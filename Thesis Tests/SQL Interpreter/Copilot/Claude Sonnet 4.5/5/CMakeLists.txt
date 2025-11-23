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

# Include directories
target_include_directories(minisql PRIVATE src)

# Link pthread for HTTP server (if needed)
if(UNIX)
    target_link_libraries(minisql pthread)
endif()
