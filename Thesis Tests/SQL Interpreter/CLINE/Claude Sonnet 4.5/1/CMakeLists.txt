cmake_minimum_required(VERSION 3.10)
project(minisql VERSION 1.0)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Source files
set(SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Storage.cpp
    src/Engine.cpp
    src/HttpServer.cpp
)

# Header files (for IDE integration)
set(HEADERS
    src/Lexer.h
    src/Parser.h
    src/Ast.h
    src/Storage.h
    src/Engine.h
    src/HttpServer.h
    src/Utils.h
)

# Create executable
add_executable(minisql ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(minisql PRIVATE src)

# Link libraries (none needed - pure C++)
# On some systems, we might need to link pthread
if(UNIX AND NOT APPLE)
    target_link_libraries(minisql pthread)
endif()

# Installation
install(TARGETS minisql DESTINATION bin)

# Print build information
message(STATUS "Building MiniSQL Interpreter")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
