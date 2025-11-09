#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

Server::Server(int port) 
    : port_(port), serverSock_(-1), fileHandler_("public"), uploadHandler_("Data") {}

Server::~Server() {
    if (serverSock_ != -1) {
        close(serverSock_);
    }
}

bool Server::setupSocket() {
    // Create socket
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Set socket options (allow reuse of address)
    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        return false;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSock_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        return false;
    }
    
    // Listen for connections
    if (listen(serverSock_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        return false;
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
    return true;
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char> buffer;
    char chunk[4096];
    ssize_t bytesRead;
    
    // Read data in chunks until we have the complete request
    while (true) {
        bytesRead = recv(clientSock, chunk, sizeof(chunk), 0);
        
        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data available right now
                break;
            }
            std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
            break;
        } else if (bytesRead == 0) {
            // Connection closed
            break;
        }
        
        // Append chunk to buffer
        buffer.insert(buffer.end(), chunk, chunk + bytesRead);
        
        // Check if we have complete headers
        std::string bufferStr(buffer.begin(), buffer.end());
        size_t headerEnd = bufferStr.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = bufferStr.find("\n\n");
        }
        
        if (headerEnd != std::string::npos) {
            // We have headers, check Content-Length
            size_t clPos = bufferStr.find("Content-Length:");
            if (clPos == std::string::npos) {
                clPos = bufferStr.find("content-length:");
            }
            
            if (clPos != std::string::npos) {
                // Extract Content-Length value
                size_t clStart = bufferStr.find(":", clPos) + 1;
                size_t clEnd = bufferStr.find("\r\n", clStart);
                if (clEnd == std::string::npos) {
                    clEnd = bufferStr.find("\n", clStart);
                }
                
                std::string clStr = bufferStr.substr(clStart, clEnd - clStart);
                // Trim whitespace
                size_t firstNonSpace = clStr.find_first_not_of(" \t");
                if (firstNonSpace != std::string::npos) {
                    clStr = clStr.substr(firstNonSpace);
                }
                
                try {
                    size_t contentLength = std::stoull(clStr);
                    size_t headerLength = (bufferStr.find("\r\n\r\n") != std::string::npos) ? 
                                         bufferStr.find("\r\n\r\n") + 4 : 
                                         bufferStr.find("\n\n") + 2;
                    size_t expectedTotal = headerLength + contentLength;
                    
                    if (buffer.size() >= expectedTotal) {
                        // Complete request received
                        break;
                    }
                } catch (...) {
                    // Invalid Content-Length, break
                    break;
                }
            } else {
                // No Content-Length (e.g., GET request), we're done
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
            if (errno != EPIPE) {
                std::cerr << "Error sending response: " << strerror(errno) << std::endl;
            }
            break;
        }
        totalSent += sent;
    }
}

HttpResponse Server::route(const HttpRequest& request) {
    std::string method = request.getMethod();
    std::string path = request.getPath();
    
    if (method == "GET") {
        // Serve static files
        return fileHandler_.serveFile(path);
    } else if (method == "POST" && path == "/") {
        // Handle file upload
        std::string contentType = request.getHeader("content-type");
        return uploadHandler_.handle(request.getBody(), contentType);
    }
    
    return HttpResponse::notFound("Endpoint not found");
}

void Server::handleClient(int clientSock) {
    // Read request
    std::vector<char> rawData = readRequest(clientSock);
    
    if (rawData.empty()) {
        close(clientSock);
        return;
    }
    
    // Parse request
    HttpRequest request;
    if (!request.parse(rawData)) {
        HttpResponse response = HttpResponse::badRequest("Invalid request");
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, responseData);
        close(clientSock);
        return;
    }
    
    // Route and generate response
    HttpResponse response = route(request);
    std::vector<char> responseData = response.build();
    
    // Send response
    sendResponse(clientSock, responseData);
    
    // Close connection
    close(clientSock);
}

void Server::run() {
    if (!setupSocket()) {
        return;
    }
    
    std::cout << "Waiting for connections..." << std::endl;
    
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSock = accept(serverSock_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        std::cout << "Client connected" << std::endl;
        
        // Handle client request
        handleClient(clientSock);
        
        std::cout << "Client disconnected" << std::endl;
    }
}
