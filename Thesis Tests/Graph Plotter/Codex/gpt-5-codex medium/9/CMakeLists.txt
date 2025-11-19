cmake_minimum_required(VERSION 3.10)
project(GraphPlotServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES
    "src/*.cpp"
)

add_executable(graph_server ${SOURCES})
target_include_directories(graph_server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(UNIX)
    target_link_libraries(graph_server PRIVATE pthread)
endif()
