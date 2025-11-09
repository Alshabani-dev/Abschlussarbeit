cmake_minimum_required(VERSION 3.10)
project(WebServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
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

# Link pthread library for threading support
target_link_libraries(webserver pthread)

# Set output directory
set_target_properties(webserver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
