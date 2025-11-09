#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <csignal>
#include <algorithm>

Server::Server(int port) : port_(port), serverSocket_(-1) {
    // Ignore SIGPIPE to prevent crashes when clients disconnect
    std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSocket_ >= 0) {
        close(serverSocket_);
    }
}

void Server::setupSocket() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to set socket options");
    }

    // Bind socket
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to bind socket");
    }

    // Listen for connections
    if (listen(serverSocket_, 10) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Server listening on port " << port_ << std::endl;
}

void Server::run() {
    setupSocket();

    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSock < 0) {
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
        }

        handleClient(clientSock);
        close(clientSock);
    }
}

bool Server::isRequestComplete(const std::vector<char>& buffer, size_t& contentLength, size_t& headerEnd) {
    // Convert buffer to string for header parsing only
    std::string bufferStr(buffer.begin(), buffer.end());
    
    // Find end of headers (supports both \r\n\r\n and \n\n)
    headerEnd = bufferStr.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = bufferStr.find("\n\n");
        if (headerEnd == std::string::npos) {
            return false; // Headers not complete yet
        }
        headerEnd += 2;
    } else {
        headerEnd += 4;
    }

    // Extract headers
    std::string headers = bufferStr.substr(0, headerEnd);
    
    // Check for Content-Length
    size_t clPos = headers.find("Content-Length:");
    if (clPos == std::string::npos) {
        clPos = headers.find("content-length:");
    }
    
    if (clPos != std::string::npos) {
        size_t clStart = clPos + 15; // Length of "Content-Length:"
        while (clStart < headers.size() && headers[clStart] == ' ') clStart++;
        size_t clEnd = headers.find('\r', clStart);
        if (clEnd == std::string::npos) {
            clEnd = headers.find('\n', clStart);
        }
        if (clEnd != std::string::npos) {
            contentLength = std::stoul(headers.substr(clStart, clEnd - clStart));
            return buffer.size() >= headerEnd + contentLength;
        }
    }
    
    // No Content-Length, headers complete is enough
    contentLength = 0;
    return true;
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char> buffer;
    char chunk[4096];
    size_t contentLength = 0;
    size_t headerEnd = 0;

    while (true) {
        ssize_t bytesRead = recv(clientSock, chunk, sizeof(chunk), 0);
        
        if (bytesRead <= 0) {
            break;
        }

        buffer.insert(buffer.end(), chunk, chunk + bytesRead);

        if (isRequestComplete(buffer, contentLength, headerEnd)) {
            break;
        }
    }

    return buffer;
}

void Server::handleClient(int clientSock) {
    try {
        std::vector<char> requestData = readRequest(clientSock);
        
        if (requestData.empty()) {
            return;
        }

        routeRequest(clientSock, requestData);
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }
}

void Server::routeRequest(int clientSock, const std::vector<char>& requestData) {
    HttpRequest request(requestData);
    HttpResponse response;
    FileHandler fileHandler;
    UploadHandler uploadHandler;

    std::string method = request.getMethod();
    std::string path = request.getPath();

    if (method == "GET" && path == "/") {
        // Serve index.html
        std::string result = fileHandler.serveFile("public/index.html");
        if (result.find("404") != std::string::npos) {
            response.setStatus(404, "Not Found");
            response.setBody(std::vector<char>(result.begin(), result.end()));
        } else {
            response.setStatus(200, "OK");
            response.setHeader("Content-Type", "text/html");
            response.setBody(std::vector<char>(result.begin(), result.end()));
        }
    } else if (method == "GET" && path == "/styles.css") {
        // Serve CSS file
        std::string result = fileHandler.serveFile("public/styles.css");
        if (result.find("404") != std::string::npos) {
            response.setStatus(404, "Not Found");
            response.setBody(std::vector<char>(result.begin(), result.end()));
        } else {
            response.setStatus(200, "OK");
            response.setHeader("Content-Type", "text/css");
            response.setBody(std::vector<char>(result.begin(), result.end()));
        }
    } else if (method == "GET" && path == "/scripts.js") {
        // Serve JavaScript file
        std::string result = fileHandler.serveFile("public/scripts.js");
        if (result.find("404") != std::string::npos) {
            response.setStatus(404, "Not Found");
            response.setBody(std::vector<char>(result.begin(), result.end()));
        } else {
            response.setStatus(200, "OK");
            response.setHeader("Content-Type", "application/javascript");
            response.setBody(std::vector<char>(result.begin(), result.end()));
        }
    } else if (method == "POST" && path == "/") {
        // Handle file upload
        std::string contentType = request.getHeader("Content-Type");
        std::vector<char> body = request.getBody();
        std::string result = uploadHandler.handle(body, contentType);
        
        if (result.find("200") != std::string::npos) {
            response.setStatus(200, "OK");
        } else {
            response.setStatus(400, "Bad Request");
        }
        response.setBody(std::vector<char>(result.begin(), result.end()));
    } else {
        response.setStatus(404, "Not Found");
        std::string notFound = "404 Not Found";
        response.setBody(std::vector<char>(notFound.begin(), notFound.end()));
    }

    // Send response
    std::vector<char> responseData = response.build();
    send(clientSock, responseData.data(), responseData.size(), MSG_NOSIGNAL);
}
