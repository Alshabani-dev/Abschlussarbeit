cmake_minimum_required(VERSION 3.10)
project(webserver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

option(ENABLE_ASAN "Enable AddressSanitizer instrumentation" OFF)

add_executable(webserver
    src/main.cpp
    src/Server.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/FileHandler.cpp
    src/UploadHandler.cpp
)

target_include_directories(webserver PRIVATE src)
target_compile_options(webserver PRIVATE -Wall -Wextra -Wpedantic -g)

if(ENABLE_ASAN)
  target_compile_options(webserver PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(webserver PRIVATE -fsanitize=address)
endif()
