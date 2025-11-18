cmake_minimum_required(VERSION 3.10)
project(minisql LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SRC_FILES
    src/*.cpp
)

add_executable(minisql ${SRC_FILES})

target_include_directories(minisql PRIVATE src)

if(MSVC)
    target_compile_options(minisql PRIVATE /W4 /permissive-)
else()
    target_compile_options(minisql PRIVATE -Wall -Wextra -pedantic)
endif()
