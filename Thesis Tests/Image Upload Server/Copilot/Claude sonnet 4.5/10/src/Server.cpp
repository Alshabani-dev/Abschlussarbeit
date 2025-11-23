#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <csignal>

Server::Server(int port) : port_(port), serverSocket_(-1) {
    // Ignore SIGPIPE to prevent crashes when client disconnects
    std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSocket_ >= 0) {
        close(serverSocket_);
    }
}

void Server::setupSocket() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }
    
    // Bind socket
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
    
    // Listen
    if (listen(serverSocket_, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
}

void Server::run() {
    setupSocket();
    
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSock = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        // Handle client in the same thread (simple synchronous approach)
        handleClient(clientSock);
        close(clientSock);
        clientBuffers_.erase(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    std::vector<char> requestData = readRequest(clientSock);
    
    if (requestData.empty()) {
        return;
    }
    
    // Parse request
    HttpRequest request;
    if (!request.parse(requestData)) {
        // Send 400 Bad Request
        HttpResponse response;
        response.setStatus(400, "Bad Request");
        response.setHeader("Connection", "close");
        response.setBody("400 Bad Request");
        sendResponse(clientSock, response.build());
        return;
    }
    
    // Route request
    std::vector<char> responseData = routeRequest(
        request.getMethod(),
        request.getPath(),
        request.getHeader("Content-Type"),
        request.getBody()
    );
    
    sendResponse(clientSock, responseData);
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char>& buffer = clientBuffers_[clientSock];
    char tempBuffer[4096];
    
    while (true) {
        ssize_t bytesRead = recv(clientSock, tempBuffer, sizeof(tempBuffer), 0);
        
        if (bytesRead <= 0) {
            break;
        }
        
        buffer.insert(buffer.end(), tempBuffer, tempBuffer + bytesRead);
        
        // Check if we have a complete request
        if (isRequestComplete(buffer)) {
            return buffer;
        }
    }
    
    return buffer;
}

bool Server::isRequestComplete(const std::vector<char>& buffer) {
    if (buffer.empty()) return false;
    
    // Convert to string to find header end
    std::string bufferStr(buffer.begin(), buffer.end());
    
    // Find header end
    size_t headerEnd = bufferStr.find("\r\n\r\n");
    size_t headerLength = 4;
    
    if (headerEnd == std::string::npos) {
        headerEnd = bufferStr.find("\n\n");
        if (headerEnd == std::string::npos) {
            return false; // Headers not complete
        }
        headerLength = 2;
    }
    
    // Check if it's a GET/HEAD request (no body expected)
    if (bufferStr.find("GET ") == 0 || bufferStr.find("HEAD ") == 0) {
        return true;
    }
    
    // For POST requests, check Content-Length
    size_t contentLength = getContentLength(buffer);
    size_t bodyStart = headerEnd + headerLength;
    size_t bodyReceived = buffer.size() - bodyStart;
    
    return bodyReceived >= contentLength;
}

size_t Server::getContentLength(const std::vector<char>& buffer) {
    std::string bufferStr(buffer.begin(), buffer.end());
    
    // Find Content-Length header (case-insensitive)
    size_t pos = bufferStr.find("Content-Length:");
    if (pos == std::string::npos) {
        pos = bufferStr.find("content-length:");
    }
    
    if (pos == std::string::npos) {
        return 0;
    }
    
    // Extract the value
    pos += 15; // Length of "Content-Length:"
    size_t endPos = bufferStr.find("\r\n", pos);
    if (endPos == std::string::npos) {
        endPos = bufferStr.find("\n", pos);
    }
    
    if (endPos == std::string::npos) {
        return 0;
    }
    
    std::string lengthStr = bufferStr.substr(pos, endPos - pos);
    
    // Trim whitespace
    size_t start = lengthStr.find_first_not_of(" \t");
    if (start != std::string::npos) {
        lengthStr = lengthStr.substr(start);
    }
    
    try {
        return std::stoul(lengthStr);
    } catch (...) {
        return 0;
    }
}

void Server::sendResponse(int clientSock, const std::vector<char>& response) {
    size_t totalSent = 0;
    while (totalSent < response.size()) {
        ssize_t sent = send(clientSock, response.data() + totalSent, 
                           response.size() - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            break;
        }
        totalSent += sent;
    }
}

std::vector<char> Server::routeRequest(const std::string& method, const std::string& path,
                                       const std::string& contentType, const std::vector<char>& body) {
    HttpResponse response;
    FileHandler fileHandler;
    
    // Handle POST (file upload)
    if (method == "POST" && path == "/") {
        UploadHandler uploadHandler;
        std::string result = uploadHandler.handle(body, contentType);
        
        if (result.find("200") == 0) {
            response.setStatus(200, "OK");
            response.setHeader("Content-Type", "text/plain");
            response.setBody("File uploaded successfully");
        } else if (result.find("400") == 0) {
            response.setStatus(400, "Bad Request");
            response.setHeader("Content-Type", "text/plain");
            response.setBody(result);
        } else {
            response.setStatus(500, "Internal Server Error");
            response.setHeader("Content-Type", "text/plain");
            response.setBody(result);
        }
        
        response.setHeader("Connection", "close");
        return response.build();
    }
    
    // Handle GET requests for static files
    if (method == "GET") {
        std::string filePath;
        
        if (path == "/") {
            filePath = "public/index.html";
        } else {
            filePath = "public" + path;
        }
        
        std::vector<char> content;
        if (fileHandler.readFile(filePath, content)) {
            response.setStatus(200, "OK");
            response.setHeader("Content-Type", fileHandler.getMimeType(filePath));
            response.setHeader("Connection", "close");
            response.setBody(content);
        } else {
            response.setStatus(404, "Not Found");
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Connection", "close");
            response.setBody("404 Not Found");
        }
        
        return response.build();
    }
    
    // Method not allowed
    response.setStatus(405, "Method Not Allowed");
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Connection", "close");
    response.setBody("405 Method Not Allowed");
    return response.build();
}
