cmake_minimum_required(VERSION 3.10)
project(WebServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Executable
add_executable(webserver ${SOURCES})

# Include directories
target_include_directories(webserver PRIVATE src)

# Link libraries
target_link_libraries(webserver PRIVATE pthread)
