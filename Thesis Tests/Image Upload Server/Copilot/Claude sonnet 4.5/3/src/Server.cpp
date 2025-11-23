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
#include <arpa/inet.h>
#include <fcntl.h>
#include <csignal>

Server::Server(int port) : port_(port), serverSocket_(-1) {
    // Ignore SIGPIPE to prevent crashes when client disconnects
    std::signal(SIGPIPE, SIG_IGN);
    
    // Create socket
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
    
    // Listen
    if (listen(serverSocket_, 10) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to listen on socket");
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
}

Server::~Server() {
    if (serverSocket_ >= 0) {
        close(serverSocket_);
    }
}

void Server::run() {
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSock = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        std::cout << "Client connected from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
        
        // Handle client (could be forked or threaded for concurrent handling)
        handleClient(clientSock);
        
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    // Read full request
    std::vector<char> requestData = readRequest(clientSock);
    
    if (requestData.empty()) {
        std::cerr << "Failed to read request" << std::endl;
        return;
    }
    
    // Parse request
    HttpRequest request;
    if (!request.parse(requestData)) {
        std::cerr << "Failed to parse request" << std::endl;
        
        // Send 400 Bad Request
        HttpResponse response;
        response.setStatus(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain");
        response.setBody("400 Bad Request");
        
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
        return;
    }
    
    std::cout << "Request: " << request.getMethod() << " " << request.getPath() << std::endl;
    
    // Route request
    HttpResponse response;
    
    if (request.getMethod() == "GET") {
        // Handle static file serving
        FileHandler fileHandler;
        int statusCode;
        std::string statusMessage, contentType;
        std::vector<char> body;
        
        fileHandler.serveFile(request.getPath(), statusCode, statusMessage, contentType, body);
        
        response.setStatus(statusCode, statusMessage);
        response.setHeader("Content-Type", contentType);
        response.setBody(body);
        
    } else if (request.getMethod() == "POST") {
        // Handle file upload
        std::string contentType = request.getHeader("Content-Type");
        
        if (contentType.find("multipart/form-data") != std::string::npos) {
            UploadHandler uploadHandler;
            std::string result = uploadHandler.handle(request.getBody(), contentType);
            
            // Parse result (format: "CODE MESSAGE")
            size_t spacePos = result.find(' ');
            if (spacePos != std::string::npos) {
                int statusCode = std::stoi(result.substr(0, spacePos));
                std::string message = result.substr(spacePos + 1);
                
                response.setStatus(statusCode, message);
                response.setHeader("Content-Type", "text/plain");
                response.setBody(message);
            } else {
                response.setStatus(500, "Internal Server Error");
                response.setHeader("Content-Type", "text/plain");
                response.setBody("500 Internal Server Error");
            }
        } else {
            response.setStatus(400, "Bad Request");
            response.setHeader("Content-Type", "text/plain");
            response.setBody("400 Bad Request: Expected multipart/form-data");
        }
        
    } else {
        // Method not allowed
        response.setStatus(405, "Method Not Allowed");
        response.setHeader("Content-Type", "text/plain");
        response.setBody("405 Method Not Allowed");
    }
    
    // Send response
    std::vector<char> responseData = response.build();
    sendResponse(clientSock, responseData);
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char> buffer;
    char chunk[4096];
    size_t contentLength = 0;
    bool headersComplete = false;
    
    while (true) {
        ssize_t bytesRead = recv(clientSock, chunk, sizeof(chunk), 0);
        
        if (bytesRead <= 0) {
            break; // Connection closed or error
        }
        
        // Append to buffer
        buffer.insert(buffer.end(), chunk, chunk + bytesRead);
        
        // Check if request is complete
        if (isRequestComplete(buffer, contentLength)) {
            break;
        }
    }
    
    return buffer;
}

bool Server::isRequestComplete(const std::vector<char>& buffer, size_t& contentLength) {
    // Find header end
    size_t headerEnd = 0;
    size_t headerSize = 0;
    
    // Try \r\n\r\n
    for (size_t i = 0; i < buffer.size() - 3; ++i) {
        if (buffer[i] == '\r' && buffer[i+1] == '\n' && 
            buffer[i+2] == '\r' && buffer[i+3] == '\n') {
            headerEnd = i;
            headerSize = 4;
            break;
        }
    }
    
    // Try \n\n
    if (headerSize == 0) {
        for (size_t i = 0; i < buffer.size() - 1; ++i) {
            if (buffer[i] == '\n' && buffer[i+1] == '\n') {
                headerEnd = i;
                headerSize = 2;
                break;
            }
        }
    }
    
    if (headerSize == 0) {
        return false; // Headers not complete yet
    }
    
    // Extract headers
    std::string headerStr(buffer.begin(), buffer.begin() + headerEnd);
    
    // Check if this is a GET/HEAD request (no body expected)
    if (headerStr.find("GET ") == 0 || headerStr.find("HEAD ") == 0) {
        return true; // GET/HEAD requests are complete after headers
    }
    
    // Extract Content-Length
    size_t clPos = headerStr.find("Content-Length:");
    if (clPos == std::string::npos) {
        clPos = headerStr.find("content-length:");
    }
    
    if (clPos != std::string::npos) {
        size_t valueStart = clPos + 15; // Skip "Content-Length:"
        while (valueStart < headerStr.length() && 
               (headerStr[valueStart] == ' ' || headerStr[valueStart] == '\t')) {
            valueStart++;
        }
        
        size_t valueEnd = headerStr.find('\r', valueStart);
        if (valueEnd == std::string::npos) {
            valueEnd = headerStr.find('\n', valueStart);
        }
        
        if (valueEnd != std::string::npos) {
            std::string clValue = headerStr.substr(valueStart, valueEnd - valueStart);
            contentLength = std::stoull(clValue);
        }
    }
    
    // Check if we have the full body
    size_t bodyStart = headerEnd + headerSize;
    size_t currentBodySize = buffer.size() - bodyStart;
    
    return (currentBodySize >= contentLength);
}

void Server::sendResponse(int clientSock, const std::vector<char>& response) {
    size_t totalSent = 0;
    
    while (totalSent < response.size()) {
        ssize_t sent = send(clientSock, response.data() + totalSent, 
                           response.size() - totalSent, MSG_NOSIGNAL);
        
        if (sent <= 0) {
            break; // Error or connection closed
        }
        
        totalSent += sent;
    }
}
