cmake_minimum_required(VERSION 3.10)
project(WebServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.h")

# Executable
add_executable(webserver ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(webserver PRIVATE src)

# Compiler flags
target_compile_options(webserver PRIVATE -Wall -Wextra -pedantic)
