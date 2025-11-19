cmake_minimum_required(VERSION 3.10)
project(GraphPlotter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(graphserver
    src/main.cpp
    src/HttpServer.cpp
    src/PlotRenderer.cpp
    src/Image.cpp
    src/Utils.cpp)

if (UNIX)
    target_link_libraries(graphserver PRIVATE pthread)
endif()
