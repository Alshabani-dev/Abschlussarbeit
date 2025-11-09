#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>
#include <arpa/inet.h>

Server::Server(int port) : port_(port), serverSocket_(-1) {
    // Ignore SIGPIPE to prevent crashes on client disconnect
    std::signal(SIGPIPE, SIG_IGN);

    // Change working directory to project root
    if (chdir("/home/aalshabani/Desktop/Workspace/Abschlussarbeit/C++/Thesis Tests/CLINE/Mistral medium/3") != 0) {
        std::cerr << "Warning: Could not change working directory" << std::endl;
    }
}

void Server::run() {
    // Create server socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        close(serverSocket_);
        return;
    }

    // Bind socket
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(serverSocket_);
        return;
    }

    // Listen for connections
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket_);
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    // Main server loop
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSock < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        // Handle client in a separate function
        handleClient(clientSock);
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    try {
        std::string requestStr = readRequest(clientSock);
        std::vector<char> requestData(requestStr.begin(), requestStr.end());

        HttpRequest request;
        request.parse(requestData);

        HttpResponse response(200, "OK");
        FileHandler fileHandler;
        UploadHandler uploadHandler;

        if (request.getPath() == "/") {
            if (request.getMethod() == "GET") {
                // Serve index.html
                try {
                    std::vector<char> fileContent = fileHandler.readFile("public/index.html");
                    response.setBody(fileContent);
                    response.setHeader("Content-Type", fileHandler.getMimeType("public/index.html"));
                } catch (const std::exception& e) {
                    response = HttpResponse(404, "Not Found");
                    response.setBody("404 Not Found: " + std::string(e.what()));
                }
            } else if (request.getMethod() == "POST") {
                // Handle file upload
                std::string result = uploadHandler.handle(request.getBody(), request.getHeader("Content-Type"));
                response.setBody(result);
                response.setHeader("Content-Type", "text/plain");
            }
        } else {
            // Try to serve static file
            std::string filePath = "public" + request.getPath();
            try {
                if (fileHandler.fileExists(filePath)) {
                    std::vector<char> fileContent = fileHandler.readFile(filePath);
                    response.setBody(fileContent);
                    response.setHeader("Content-Type", fileHandler.getMimeType(filePath));
                } else {
                    response = HttpResponse(404, "Not Found");
                    response.setBody("404 Not Found");
                }
            } catch (const std::exception& e) {
                response = HttpResponse(404, "Not Found");
                response.setBody("404 Not Found: " + std::string(e.what()));
            }
        }

        // Set Connection header
        response.setHeader("Connection", "close");

        // Build response
        std::vector<char> responseData = response.build();
        sendResponse(clientSock, std::string(responseData.begin(), responseData.end()));
    } catch (const std::exception& e) {
        HttpResponse errorResponse(500, "Internal Server Error");
        errorResponse.setBody("500 Internal Server Error: " + std::string(e.what()));
        errorResponse.setHeader("Content-Type", "text/plain");
        errorResponse.setHeader("Connection", "close");

        std::vector<char> errorData = errorResponse.build();
        sendResponse(clientSock, std::string(errorData.begin(), errorData.end()));
    }
}

std::string Server::readRequest(int clientSock) {
    std::string request;
    char buffer[4096];
    ssize_t bytesRead;

    // First read to get headers
    while ((bytesRead = recv(clientSock, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, bytesRead);

        // Check if we have the headers
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = request.find("\n\n");
            if (headerEnd == std::string::npos) {
                continue; // Keep reading until we find the header end
            }
        }

        // Check if this is a GET or HEAD request (no body expected)
        size_t methodEnd = request.find(' ');
        if (methodEnd != std::string::npos) {
            std::string method = request.substr(0, methodEnd);
            if (method == "GET" || method == "HEAD") {
                break; // No body expected for GET/HEAD requests
            }
        }

        // For POST requests, check Content-Length
        size_t contentLengthPos = request.find("Content-Length: ");
        if (contentLengthPos != std::string::npos) {
            size_t endOfLine = request.find("\r\n", contentLengthPos);
            if (endOfLine != std::string::npos) {
                std::string contentLengthStr = request.substr(
                    contentLengthPos + 16,
                    endOfLine - (contentLengthPos + 16)
                );
                try {
                    size_t contentLength = std::stoul(contentLengthStr);
                    size_t bodyStart = headerEnd + (request[headerEnd + 2] == '\r' ? 4 : 2);
                    if (request.length() >= bodyStart + contentLength) {
                        break; // We have the full request
                    }
                } catch (...) {
                    // Ignore parsing errors, continue reading
                }
            }
        }
    }

    return request;
}

void Server::sendResponse(int clientSock, const std::string& response) {
    send(clientSock, response.c_str(), response.size(), MSG_NOSIGNAL);
}
