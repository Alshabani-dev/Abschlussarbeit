cmake_minimum_required(VERSION 3.10)
project(WebServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(webserver
    src/main.cpp
    src/Server.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/FileHandler.cpp
    src/UploadHandler.cpp
)

target_include_directories(webserver PRIVATE src)
