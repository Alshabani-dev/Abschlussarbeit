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

# Link pthread for HTTP server
target_link_libraries(minisql pthread)

# Ensure data directory exists at build time
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data)
