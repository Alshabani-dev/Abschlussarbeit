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
#include <csignal>
#include <sstream>

Server::Server(int port) : port_(port), serverSock_(-1) {
    // Ignore SIGPIPE to prevent crashes when client disconnects
    std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSock_ >= 0) {
        close(serverSock_);
    }
}

void Server::run() {
    // Create socket
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(serverSock_);
        return;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSock_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(serverSock_);
        return;
    }
    
    // Listen for connections
    if (listen(serverSock_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSock_);
        return;
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
    
    // Accept connections
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSock < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        handleClient(clientSock);
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    // Read request with proper buffering
    std::vector<char> rawData = readRequest(clientSock);
    
    if (rawData.empty()) {
        // Send 400 Bad Request
        HttpResponse response;
        response.setStatus(400, "Bad Request");
        response.setBody("Bad Request");
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
        return;
    }
    
    // Parse request
    HttpRequest request;
    if (!request.parse(rawData)) {
        HttpResponse response;
        response.setStatus(400, "Bad Request");
        response.setBody("Bad Request");
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
        return;
    }
    
    std::string method = request.getMethod();
    std::string path = request.getPath();
    
    // Route requests
    if (method == "GET") {
        // Serve static files
        std::string filepath;
        if (path == "/") {
            filepath = "public/index.html";
        } else {
            filepath = "public" + path;
        }
        
        FileHandler fileHandler;
        if (!fileHandler.fileExists(filepath)) {
            HttpResponse response;
            response.setStatus(404, "Not Found");
            response.setBody("404 Not Found");
            std::vector<char> responseData = response.build();
            sendResponse(clientSock, responseData);
            return;
        }
        
        std::vector<char> fileContent = fileHandler.readFile(filepath);
        std::string mimeType = fileHandler.getMimeType(filepath);
        
        HttpResponse response;
        response.setStatus(200, "OK");
        response.setHeader("Content-Type", mimeType);
        response.setBody(fileContent);
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
        
    } else if (method == "POST" && path == "/") {
        // Handle upload
        std::string contentType = request.getHeader("Content-Type");
        
        if (contentType.find("multipart/form-data") == std::string::npos) {
            HttpResponse response;
            response.setStatus(400, "Bad Request");
            response.setBody("Expected multipart/form-data");
            std::vector<char> responseData = response.build();
            sendResponse(clientSock, responseData);
            return;
        }
        
        UploadHandler uploadHandler;
        std::string result = uploadHandler.handle(request.getBody(), contentType);
        
        HttpResponse response;
        if (result == "200 OK") {
            response.setStatus(200, "OK");
            response.setHeader("Content-Type", "text/plain");
            response.setBody("File uploaded successfully");
        } else {
            // Parse error code
            int statusCode = 400;
            if (result.find("500") == 0) {
                statusCode = 500;
            }
            response.setStatus(statusCode, result);
            response.setBody(result);
        }
        
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
        
    } else {
        // Method not allowed or path not found
        HttpResponse response;
        response.setStatus(404, "Not Found");
        response.setBody("404 Not Found");
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
    }
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char> buffer;
    char chunk[4096];
    
    // Read data in chunks until we have a complete request
    while (true) {
        ssize_t bytesRead = recv(clientSock, chunk, sizeof(chunk), 0);
        
        if (bytesRead <= 0) {
            break;
        }
        
        buffer.insert(buffer.end(), chunk, chunk + bytesRead);
        
        // Check if we have a complete request
        HttpRequest tempRequest;
        if (tempRequest.isComplete(buffer)) {
            break;
        }
        
        // Prevent infinite buffering
        if (buffer.size() > 100 * 1024 * 1024) { // 100MB max
            return std::vector<char>();
        }
    }
    
    return buffer;
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
