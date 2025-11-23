cmake_minimum_required(VERSION 3.10)
project(webserver)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Source files
set(SOURCES
    src/main.cpp
    src/Server.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/FileHandler.cpp
    src/UploadHandler.cpp
)

# Create executable
add_executable(webserver ${SOURCES})

# Include directories
target_include_directories(webserver PRIVATE src)

# Link libraries (pthread for multi-threading if needed later)
target_link_libraries(webserver pthread)
