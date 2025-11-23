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

Server::Server(int port) 
    : port_(port), serverSocket_(-1) {
    // Ignore SIGPIPE to prevent server crashes on client disconnect
    std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSocket_ >= 0) {
        close(serverSocket_);
    }
}

void Server::setNonBlocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl F_GETFL failed" << std::endl;
        return;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl F_SETFL failed" << std::endl;
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
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
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

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char> buffer;
    char temp[4096];
    HttpRequest tempRequest;
    
    // Read data in chunks until we have a complete request
    while (true) {
        ssize_t bytesRead = recv(clientSock, temp, sizeof(temp), 0);
        
        if (bytesRead > 0) {
            buffer.insert(buffer.end(), temp, temp + bytesRead);
            
            // Check if request is complete
            if (tempRequest.isComplete(buffer)) {
                break;
            }
        } else if (bytesRead == 0) {
            // Connection closed
            break;
        } else {
            // Error or would block
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data available right now, wait a bit
                usleep(10000); // 10ms
                continue;
            } else {
                // Real error
                break;
            }
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000); // Wait 1ms and retry
                continue;
            }
            // Error sending
            break;
        }
        totalSent += sent;
    }
}

void Server::handleClient(int clientSock) {
    // Read request
    std::vector<char> requestData = readRequest(clientSock);
    
    if (requestData.empty()) {
        close(clientSock);
        return;
    }
    
    // Parse request
    HttpRequest request;
    if (!request.parse(requestData)) {
        HttpResponse response;
        response.setStatus(400, "Bad Request");
        response.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
        response.setHeader("Content-Type", "text/html");
        
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
        close(clientSock);
        return;
    }
    
    std::cout << request.getMethod() << " " << request.getPath() << std::endl;
    
    HttpResponse response;
    
    // Route based on method and path
    if (request.getMethod() == "GET") {
        // Serve static files
        FileHandler fileHandler;
        response = fileHandler.serveFile(request.getPath());
    } else if (request.getMethod() == "POST" && request.getPath() == "/") {
        // Handle file upload
        std::string contentType = request.getHeader("Content-Type");
        
        if (contentType.find("multipart/form-data") != std::string::npos) {
            UploadHandler uploadHandler;
            std::string result = uploadHandler.handle(request.getBody(), contentType);
            
            if (result == "200 OK") {
                response.setStatus(200, "OK");
                response.setBody("<html><body><script>alert('File uploaded successfully!'); window.location.href='/';</script></body></html>");
                response.setHeader("Content-Type", "text/html");
            } else {
                // Extract status code
                int statusCode = 400;
                std::string message = result;
                
                if (result.find("400") != std::string::npos) {
                    statusCode = 400;
                } else if (result.find("500") != std::string::npos) {
                    statusCode = 500;
                }
                
                response.setStatus(statusCode, message.substr(4));
                response.setBody("<html><body><script>alert('" + message + "'); window.location.href='/';</script></body></html>");
                response.setHeader("Content-Type", "text/html");
            }
        } else {
            response.setStatus(400, "Bad Request");
            response.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
            response.setHeader("Content-Type", "text/html");
        }
    } else {
        response.setStatus(404, "Not Found");
        response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
        response.setHeader("Content-Type", "text/html");
    }
    
    // Send response
    std::vector<char> responseData = response.build();
    sendResponse(clientSock, responseData);
    
    close(clientSock);
}

void Server::run() {
    setupSocket();
    
    while (true) {
        struct sockaddr_in clientAddress;
        socklen_t clientLen = sizeof(clientAddress);
        
        int clientSock = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientLen);
        
        if (clientSock < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        
        // Set client socket to non-blocking
        setNonBlocking(clientSock);
        
        // Handle client request
        handleClient(clientSock);
    }
}
