cmake_minimum_required(VERSION 3.10)
project(MinimalSQLInterpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(minisql
    src/main.cpp
    src/Engine.cpp
    src/Storage.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/HttpServer.cpp
)

# Include directories
target_include_directories(minisql PRIVATE src)

# Enable filesystem support
target_compile_options(minisql PRIVATE -std=c++17)
target_link_libraries(minisql PRIVATE stdc++fs)
