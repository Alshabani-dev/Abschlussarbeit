#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <vector>

Server::Server(int port) : port_(port), serverSocket_(-1) {
    // Ignore SIGPIPE to prevent server crashes when client disconnects
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
        throw std::runtime_error("Failed to bind socket to port " + std::to_string(port_));
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
        // Accept client connection
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSock = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        std::cout << "Client connected from " << getClientIP(clientSock) << std::endl;
        
        // Handle client request
        try {
            handleClient(clientSock);
        } catch (const std::exception& e) {
            std::cerr << "Error handling client: " << e.what() << std::endl;
        }
        
        close(clientSock);
        std::cout << "Client disconnected" << std::endl;
    }
}

void Server::handleClient(int clientSock) {
    // Read request
    std::vector<char> rawRequest = readRequest(clientSock);
    if (rawRequest.empty()) {
        std::cerr << "Empty request received" << std::endl;
        return;
    }
    
    // Parse HTTP request
    HttpRequest request;
    if (!request.parse(rawRequest)) {
        std::cerr << "Failed to parse request" << std::endl;
        HttpResponse response = HttpResponse::badRequest("Invalid HTTP request");
        sendResponse(clientSock, response.build());
        return;
    }
    
    // Check if request is complete
    if (!request.isComplete()) {
        std::cerr << "Incomplete request received" << std::endl;
        HttpResponse response = HttpResponse::badRequest("Incomplete request");
        sendResponse(clientSock, response.build());
        return;
    }
    
    std::cout << "Request: " << request.getMethod() << " " << request.getPath() << std::endl;
    
    // Route request
    HttpResponse response;
    
    if (request.getMethod() == "GET") {
        // Serve static files
        FileHandler fileHandler;
        response = fileHandler.handle(request.getPath());
    } else if (request.getMethod() == "POST") {
        // Handle file upload
        std::string contentType = request.getHeader("content-type");
        if (contentType.find("multipart/form-data") != std::string::npos) {
            UploadHandler uploadHandler;
            std::string result = uploadHandler.handle(request.getBody(), contentType);
            
            // Parse result to determine status code
            if (result.find("200 OK") == 0) {
                response = HttpResponse::ok(result, "text/plain");
            } else if (result.find("400") == 0) {
                response = HttpResponse::badRequest(result);
            } else {
                response = HttpResponse::internalError(result);
            }
        } else {
            response = HttpResponse::badRequest("Expected multipart/form-data");
        }
    } else {
        response = HttpResponse::badRequest("Method not supported");
    }
    
    // Send response
    sendResponse(clientSock, response.build());
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char> buffer;
    char chunk[4096];
    
    // Set socket to non-blocking mode temporarily
    int flags = fcntl(clientSock, F_GETFL, 0);
    fcntl(clientSock, F_SETFL, flags | O_NONBLOCK);
    
    // Read data in chunks with timeout
    fd_set readSet;
    struct timeval timeout;
    bool headersComplete = false;
    size_t contentLength = 0;
    size_t headerSize = 0;
    
    while (true) {
        FD_ZERO(&readSet);
        FD_SET(clientSock, &readSet);
        timeout.tv_sec = 5;  // 5 second timeout
        timeout.tv_usec = 0;
        
        int selectResult = select(clientSock + 1, &readSet, nullptr, nullptr, &timeout);
        if (selectResult <= 0) {
            break;  // Timeout or error
        }
        
        ssize_t bytesRead = recv(clientSock, chunk, sizeof(chunk), 0);
        if (bytesRead <= 0) {
            break;  // Connection closed or error
        }
        
        buffer.insert(buffer.end(), chunk, chunk + bytesRead);
        
        // Check if headers are complete
        if (!headersComplete) {
            std::string bufferStr(buffer.begin(), buffer.end());
            size_t headerEnd = bufferStr.find("\r\n\r\n");
            if (headerEnd == std::string::npos) {
                headerEnd = bufferStr.find("\n\n");
                if (headerEnd != std::string::npos) {
                    headerSize = headerEnd + 2;
                    headersComplete = true;
                }
            } else {
                headerSize = headerEnd + 4;
                headersComplete = true;
            }
            
            if (headersComplete) {
                // Extract Content-Length
                std::string headers = bufferStr.substr(0, headerSize);
                size_t clPos = headers.find("Content-Length:");
                if (clPos == std::string::npos) {
                    clPos = headers.find("content-length:");
                }
                
                if (clPos != std::string::npos) {
                    size_t lineEnd = headers.find("\n", clPos);
                    std::string clLine = headers.substr(clPos, lineEnd - clPos);
                    size_t colonPos = clLine.find(':');
                    if (colonPos != std::string::npos) {
                        std::string clValue = clLine.substr(colonPos + 1);
                        // Trim whitespace
                        clValue.erase(0, clValue.find_first_not_of(" \t\r\n"));
                        clValue.erase(clValue.find_last_not_of(" \t\r\n") + 1);
                        contentLength = std::stoull(clValue);
                    }
                } else {
                    // No Content-Length, request is complete
                    break;
                }
            }
        }
        
        // Check if we have received all data
        if (headersComplete) {
            size_t bodySize = buffer.size() - headerSize;
            if (bodySize >= contentLength) {
                break;  // Request complete
            }
        }
    }
    
    // Restore blocking mode
    fcntl(clientSock, F_SETFL, flags);
    
    return buffer;
}

void Server::sendResponse(int clientSock, const std::vector<char>& response) {
    size_t totalSent = 0;
    while (totalSent < response.size()) {
        ssize_t sent = send(clientSock, response.data() + totalSent, 
                           response.size() - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            std::cerr << "Failed to send response" << std::endl;
            break;
        }
        totalSent += sent;
    }
}

std::string Server::getClientIP(int clientSock) {
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    
    if (getpeername(clientSock, (struct sockaddr*)&addr, &addrLen) == 0) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
        return std::string(ip);
    }
    
    return "unknown";
}
