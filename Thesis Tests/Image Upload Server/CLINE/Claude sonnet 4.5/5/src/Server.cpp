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
#include <errno.h>

Server::Server(int port) : port_(port), serverSocket_(-1) {
    // Ignore SIGPIPE to prevent server crashes on client disconnect
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
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket to port " + std::to_string(port_));
    }
    
    // Listen for connections
    if (listen(serverSocket_, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char>& buffer = clientBuffers_[clientSock];
    char chunk[4096];
    
    while (true) {
        ssize_t bytesRead = recv(clientSock, chunk, sizeof(chunk), 0);
        
        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data available right now
                break;
            }
            // Error occurred
            return std::vector<char>();
        }
        
        if (bytesRead == 0) {
            // Connection closed
            return std::vector<char>();
        }
        
        // Append to buffer
        buffer.insert(buffer.end(), chunk, chunk + bytesRead);
        
        // Check if request is complete
        HttpRequest tempRequest;
        if (tempRequest.isComplete(buffer)) {
            break;
        }
    }
    
    return buffer;
}

void Server::sendResponse(int clientSock, const std::vector<char>& response) {
    size_t totalSent = 0;
    
    while (totalSent < response.size()) {
        ssize_t bytesSent = send(clientSock, response.data() + totalSent, 
                                  response.size() - totalSent, MSG_NOSIGNAL);
        
        if (bytesSent < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                // Client disconnected
                break;
            }
            break;
        }
        
        totalSent += bytesSent;
    }
}

void Server::handleClient(int clientSock) {
    // Read request
    std::vector<char> requestData = readRequest(clientSock);
    
    if (requestData.empty()) {
        close(clientSock);
        clientBuffers_.erase(clientSock);
        return;
    }
    
    // Parse request
    HttpRequest request;
    if (!request.parse(requestData)) {
        // Send 400 Bad Request
        HttpResponse response;
        response.setStatusCode(400);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Connection", "close");
        response.setBody("Bad Request");
        
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
        
        close(clientSock);
        clientBuffers_.erase(clientSock);
        return;
    }
    
    // Route the request
    HttpResponse response;
    
    if (request.getMethod() == "GET") {
        // Serve static files
        FileHandler fileHandler("public");
        
        std::string path = request.getPath();
        if (path == "/") {
            path = "/index.html";
        }
        
        std::vector<char> content;
        std::string mimeType;
        
        if (fileHandler.serve(path, content, mimeType)) {
            response.setStatusCode(200);
            response.setHeader("Content-Type", mimeType);
            response.setHeader("Connection", "close");
            response.setBody(content);
        } else {
            response.setStatusCode(404);
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Connection", "close");
            response.setBody("404 Not Found");
        }
    } 
    else if (request.getMethod() == "POST") {
        // Handle file upload
        std::string contentType = request.getHeader("content-type");
        
        if (contentType.find("multipart/form-data") != std::string::npos) {
            UploadHandler uploadHandler("Data");
            std::string result = uploadHandler.handle(request.getBody(), contentType);
            
            // Parse result (status code and message)
            int statusCode = 200;
            std::string message = "Upload successful";
            
            if (result.find("400") != std::string::npos) {
                statusCode = 400;
                message = result;
            } else if (result.find("500") != std::string::npos) {
                statusCode = 500;
                message = result;
            }
            
            response.setStatusCode(statusCode);
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Connection", "close");
            response.setBody(message);
        } else {
            response.setStatusCode(400);
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Connection", "close");
            response.setBody("400 Bad Request: Expected multipart/form-data");
        }
    }
    else {
        response.setStatusCode(405);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Connection", "close");
        response.setBody("405 Method Not Allowed");
    }
    
    // Send response
    std::vector<char> responseData = response.build();
    sendResponse(clientSock, responseData);
    
    // Close connection
    close(clientSock);
    clientBuffers_.erase(clientSock);
}

void Server::run() {
    setupSocket();
    
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSock = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSock < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        // Handle client request
        handleClient(clientSock);
    }
}
