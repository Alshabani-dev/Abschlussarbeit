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
#include <algorithm>

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
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    // Set socket to reuse address
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }
    
    // Bind socket
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
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
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSock = accept(serverSocket_, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        // Set socket to non-blocking mode
        int flags = fcntl(clientSock, F_GETFL, 0);
        fcntl(clientSock, F_SETFL, flags | O_NONBLOCK);
        
        handleClient(clientSock);
        close(clientSock);
        clientBuffers_.erase(clientSock);
    }
}

bool Server::readRequest(int clientSock, std::vector<char>& fullRequest) {
    auto& clientBuf = clientBuffers_[clientSock];
    char buffer[4096];
    
    while (true) {
        ssize_t bytesRead = recv(clientSock, buffer, sizeof(buffer), 0);
        
        if (bytesRead > 0) {
            clientBuf.buffer.insert(clientBuf.buffer.end(), buffer, buffer + bytesRead);
            
            // Parse headers if not yet parsed
            if (!clientBuf.headersParsed) {
                std::string bufferStr(clientBuf.buffer.begin(), clientBuf.buffer.end());
                
                // Find end of headers
                size_t headersEnd = bufferStr.find("\r\n\r\n");
                if (headersEnd == std::string::npos) {
                    headersEnd = bufferStr.find("\n\n");
                    if (headersEnd != std::string::npos) {
                        clientBuf.headersEndPos = headersEnd + 2;
                    }
                } else {
                    clientBuf.headersEndPos = headersEnd + 4;
                }
                
                if (clientBuf.headersEndPos > 0) {
                    clientBuf.headersParsed = true;
                    
                    // Extract Content-Length
                    std::string headers = bufferStr.substr(0, clientBuf.headersEndPos);
                    size_t clPos = headers.find("Content-Length:");
                    if (clPos == std::string::npos) {
                        clPos = headers.find("content-length:");
                    }
                    
                    if (clPos != std::string::npos) {
                        size_t clStart = clPos + 15;
                        size_t clEnd = headers.find("\r\n", clStart);
                        if (clEnd == std::string::npos) {
                            clEnd = headers.find("\n", clStart);
                        }
                        if (clEnd != std::string::npos) {
                            std::string clValue = headers.substr(clStart, clEnd - clStart);
                            // Trim whitespace
                            clValue.erase(0, clValue.find_first_not_of(" \t"));
                            clValue.erase(clValue.find_last_not_of(" \t\r\n") + 1);
                            clientBuf.contentLength = std::stoull(clValue);
                        }
                    }
                }
            }
            
            // Check if we have complete request
            if (clientBuf.headersParsed) {
                size_t totalExpected = clientBuf.headersEndPos + clientBuf.contentLength;
                if (clientBuf.buffer.size() >= totalExpected) {
                    fullRequest = clientBuf.buffer;
                    return true;
                }
            }
        } else if (bytesRead == 0) {
            // Connection closed
            return false;
        } else {
            // EAGAIN or EWOULDBLOCK - no more data available right now
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Wait a bit and try again
                usleep(10000); // 10ms
                continue;
            } else {
                // Error
                return false;
            }
        }
    }
}

void Server::handleClient(int clientSock) {
    std::vector<char> requestData;
    
    if (!readRequest(clientSock, requestData)) {
        return;
    }
    
    processRequest(clientSock, requestData);
}

void Server::processRequest(int clientSock, const std::vector<char>& requestData) {
    HttpRequest request(requestData);
    HttpResponse response;
    
    std::string method = request.getMethod();
    std::string path = request.getPath();
    
    if (method == "GET") {
        if (path == "/") {
            path = "/index.html";
        }
        
        FileHandler fileHandler;
        std::vector<char> fileContent;
        
        if (fileHandler.readFile("public" + path, fileContent)) {
            std::string contentType = getContentType(path);
            response.setStatus(200, "OK");
            response.setHeader("Content-Type", contentType);
            response.setHeader("Content-Length", std::to_string(fileContent.size()));
            response.setHeader("Connection", "close");
            response.setBody(fileContent);
        } else {
            std::string errorMsg = "404 Not Found";
            response.setStatus(404, "Not Found");
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Content-Length", std::to_string(errorMsg.size()));
            response.setHeader("Connection", "close");
            response.setBody(std::vector<char>(errorMsg.begin(), errorMsg.end()));
        }
    } else if (method == "POST" && path == "/") {
        UploadHandler uploadHandler;
        std::string contentType = request.getHeader("Content-Type");
        std::vector<char> body = request.getBody();
        
        std::string result = uploadHandler.handle(body, contentType);
        
        if (result.find("200") == 0) {
            response.setStatus(200, "OK");
        } else {
            response.setStatus(400, "Bad Request");
        }
        
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Content-Length", std::to_string(result.size()));
        response.setHeader("Connection", "close");
        response.setBody(std::vector<char>(result.begin(), result.end()));
    } else {
        std::string errorMsg = "405 Method Not Allowed";
        response.setStatus(405, "Method Not Allowed");
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Content-Length", std::to_string(errorMsg.size()));
        response.setHeader("Connection", "close");
        response.setBody(std::vector<char>(errorMsg.begin(), errorMsg.end()));
    }
    
    std::vector<char> responseData = response.build();
    send(clientSock, responseData.data(), responseData.size(), MSG_NOSIGNAL);
}

std::string Server::getContentType(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".png") != std::string::npos) return "image/png";
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) return "image/jpeg";
    if (path.find(".gif") != std::string::npos) return "image/gif";
    if (path.find(".bmp") != std::string::npos) return "image/bmp";
    return "application/octet-stream";
}
