#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

Server::Server(int port) : port_(port) {
    ignoreSigpipe();
}

void Server::ignoreSigpipe() {
    std::signal(SIGPIPE, SIG_IGN);
}

void Server::run() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error opening socket" << std::endl;
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return;
    }

    listen(serverSocket_, 5);
    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        handleClient(clientSock);
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    std::string request = readRequest(clientSock);

    // Check if request is empty or invalid
    if (request.empty()) {
        HttpResponse response(400);
        response.setBody("<h1>400 Bad Request</h1><p>Empty request received</p>");
        std::string responseStr = response.toString();
        send(clientSock, responseStr.c_str(), responseStr.size(), 0);
        return;
    }

    HttpRequest httpRequest(request);

    // Handle error case from parsing
    if (httpRequest.getMethod() == "ERROR") {
        HttpResponse response(400);
        response.setBody("<h1>400 Bad Request</h1><p>Invalid request format</p>");
        std::string responseStr = response.toString();
        send(clientSock, responseStr.c_str(), responseStr.size(), 0);
        return;
    }

    HttpResponse response;
    FileHandler fileHandler;
    UploadHandler uploadHandler;

    try {
        if (httpRequest.getMethod() == "GET") {
            std::string path = "public" + httpRequest.getPath();
            if (path.back() == '/') {
                path += "index.html";
            }

            if (fileHandler.fileExists(path)) {
                std::vector<char> fileContent = fileHandler.readFile(path);
                response.setBody(fileContent);
                response.setHeader("Content-Type", fileHandler.getMimeType(path));
            } else {
                response.setStatusCode(404);
                response.setBody("<h1>404 Not Found</h1>");
            }
        } else if (httpRequest.getMethod() == "POST" && httpRequest.getPath() == "/") {
            std::string contentType = httpRequest.getHeader("Content-Type");
            std::string result = uploadHandler.handle(httpRequest.getBody(), contentType);

            if (result.find("200 OK") != std::string::npos) {
                response.setStatusCode(200);
                response.setBody("<html><body><h1>Upload Successful</h1><p>" + result + "</p></body></html>");
            } else if (result.find("400 Bad Request") != std::string::npos) {
                response.setStatusCode(400);
                response.setBody("<html><body><h1>Error</h1><p>" + result + "</p></body></html>");
            } else {
                response.setStatusCode(500);
                response.setBody("<html><body><h1>Error</h1><p>" + result + "</p></body></html>");
            }
        } else {
            response.setStatusCode(404);
            response.setBody("<h1>404 Not Found</h1>");
        }

        std::string responseStr = response.toString();
        send(clientSock, responseStr.c_str(), responseStr.size(), 0);
    } catch (const std::exception& e) {
        HttpResponse errorResponse(500);
        errorResponse.setBody("<h1>500 Internal Server Error</h1><p>" + std::string(e.what()) + "</p>");
        std::string responseStr = errorResponse.toString();
        send(clientSock, responseStr.c_str(), responseStr.size(), 0);
    }
}

std::string Server::readRequest(int clientSock) {
    std::vector<char> buffer(4096);
    std::string request;
    ssize_t bytesRead;

    try {
        // First read to get headers
        bytesRead = recv(clientSock, buffer.data(), buffer.size(), 0);
        if (bytesRead <= 0) {
            std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
            return "";
        }
        request.append(buffer.data(), bytesRead);

        // Check if this is a GET request (no body)
        if (request.find("GET ") != std::string::npos) {
            return request;
        }

        // For POST requests, check Content-Length
        size_t contentLengthPos = request.find("Content-Length: ");
        if (contentLengthPos != std::string::npos) {
            size_t endPos = request.find("\r\n", contentLengthPos);
            if (endPos == std::string::npos) {
                endPos = request.find("\n", contentLengthPos);
            }

            if (endPos != std::string::npos) {
                std::string contentLengthStr = request.substr(
                    contentLengthPos + 16,
                    endPos - (contentLengthPos + 16)
                );

                try {
                    size_t contentLength = std::stoul(contentLengthStr);
                    size_t headerEnd = request.find("\r\n\r\n");
                    if (headerEnd == std::string::npos) {
                        headerEnd = request.find("\n\n");
                        if (headerEnd != std::string::npos) {
                            headerEnd += 2;
                        }
                    } else {
                        headerEnd += 4;
                    }

                    // If we already have the full body, return
                    if (headerEnd != std::string::npos && request.length() >= headerEnd + contentLength) {
                        return request;
                    }

                    // Otherwise, read the remaining body
                    size_t bodyRead = headerEnd != std::string::npos ? request.length() - headerEnd : 0;
                    while (bodyRead < contentLength) {
                        bytesRead = recv(clientSock, buffer.data(), std::min(buffer.size(), contentLength - bodyRead), 0);
                        if (bytesRead <= 0) {
                            std::cerr << "Error reading request body: " << strerror(errno) << std::endl;
                            return "";
                        }
                        request.append(buffer.data(), bytesRead);
                        bodyRead += bytesRead;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing Content-Length: " << e.what() << std::endl;
                    return "";
                }
            }
        }

        return request;
    } catch (const std::exception& e) {
        std::cerr << "Exception while reading request: " << e.what() << std::endl;
        return "";
    }
}
