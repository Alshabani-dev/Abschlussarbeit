cmake_minimum_required(VERSION 3.10)
project(MinimalSQLInterpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(minisql ${SOURCES})

# Include directories
target_include_directories(minisql PRIVATE src)

# Link filesystem library
target_link_libraries(minisql PRIVATE stdc++fs)
