cmake_minimum_required(VERSION 3.10)
project(GraphPlotter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Source files
set(SOURCES
    src/main.cpp
    src/HttpServer.cpp
    src/PlotRenderer.cpp
    src/Image.cpp
    src/Utils.cpp
)

# Create executable
add_executable(graphserver ${SOURCES})

# Link threading library (required for sockets on some systems)
find_package(Threads REQUIRED)
