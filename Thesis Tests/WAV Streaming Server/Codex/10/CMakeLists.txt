cmake_minimum_required(VERSION 3.10)
project(wavserver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

set(SOURCES
    src/main.cpp
    src/Server.cpp
)

add_executable(wavserver ${SOURCES})
target_include_directories(wavserver PRIVATE src)
