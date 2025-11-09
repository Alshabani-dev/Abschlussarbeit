cmake_minimum_required(VERSION 3.10)
project(SimpleCppWebServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(webserver
    src/main.cpp
    src/Server.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/FileHandler.cpp
    src/UploadHandler.cpp
)

# Include directories
target_include_directories(webserver PRIVATE src)

# Link libraries
target_link_libraries(webserver PRIVATE pthread)
