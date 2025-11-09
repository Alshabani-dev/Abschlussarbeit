#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <csignal>

Server::Server(int port) : port_(port) {
    // Ignore SIGPIPE to prevent crashes on client disconnections
    std::signal(SIGPIPE, SIG_IGN);
}

void Server::run() {
    int server_fd, client_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        // Accept incoming connection
        if ((client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // Handle client in a separate function
        handleClient(client_sock);
        close(client_sock);
    }
}

void Server::handleClient(int clientSock) {
    // Read the request
    std::string rawRequest = readRequest(clientSock);

    // Parse the request
    HttpRequest request;
    request.parse(rawRequest);

    // Create response object
    HttpResponse response;

    // Handle different request methods
    if (request.getMethod() == "GET") {
        handleGetRequest(request, response);
    } else if (request.getMethod() == "POST") {
        handlePostRequest(request, response);
    } else {
        response.setStatus(405, "Method Not Allowed");
        response.addHeader("Content-Type", "text/plain");
        response.setBody("405 Method Not Allowed");
    }

    // Send the response
    std::string responseStr = response.toString();
    send(clientSock, responseStr.c_str(), responseStr.size(), 0);

    // If there's binary data, send it separately
    if (response.getBinaryBody().size() > 0) {
        send(clientSock, response.getBinaryBody().data(), response.getBinaryBody().size(), 0);
    }
}

void Server::handleGetRequest(const HttpRequest& request, HttpResponse& response) {
    std::string path = request.getPath();

    // Default to index.html
    if (path == "/") {
        path = "/index.html";
    }

    // Serve static files
    FileHandler fileHandler;
    std::string filePath = "public" + path;

    if (fileHandler.fileExists(filePath)) {
        std::vector<char> fileData = fileHandler.readFile(filePath);
        std::string mimeType = fileHandler.getMimeType(filePath);

        response.setStatus(200, "OK");
        response.addHeader("Content-Type", mimeType);
        response.setBody(fileData);
    } else {
        response.setStatus(404, "Not Found");
        response.addHeader("Content-Type", "text/plain");
        response.setBody("404 Not Found: " + filePath);
    }
}

void Server::handlePostRequest(const HttpRequest& request, HttpResponse& response) {
    std::string contentType = request.getHeader("Content-Type");

    // Handle file upload
    if (contentType.find("multipart/form-data") != std::string::npos) {
        UploadHandler uploadHandler;
        std::string result = uploadHandler.handle(request.getBody(), contentType);

        if (result.find("200 OK") != std::string::npos) {
            response.setStatus(200, "OK");
            response.addHeader("Content-Type", "text/plain");
            response.setBody("File uploaded successfully");
        } else {
            response.setStatus(400, "Bad Request");
            response.addHeader("Content-Type", "text/plain");
            response.setBody(result);
        }
    } else {
        response.setStatus(400, "Bad Request");
        response.addHeader("Content-Type", "text/plain");
        response.setBody("400 Bad Request: Unsupported content type");
    }
}

std::string Server::readRequest(int clientSock) {
    std::vector<char> buffer(4096);
    std::string request;
    ssize_t bytes_read;

    // Read request in chunks
    while ((bytes_read = recv(clientSock, buffer.data(), buffer.size(), 0)) > 0) {
        request.append(buffer.data(), bytes_read);

        // For GET requests, we can stop after reading headers
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = request.find("\n\n");
        }

        if (headerEnd != std::string::npos) {
            // Check if this is a GET request (no body)
            size_t methodEnd = request.find(' ');
            if (methodEnd != std::string::npos) {
                std::string method = request.substr(0, methodEnd);
                if (method == "GET") {
                    // For GET requests, we're done after headers
                    break;
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
                        size_t bodyStart = headerEnd + 4;
                        if (request.size() >= bodyStart + contentLength) {
                            // We have the full request
                            break;
                        }
                    } catch (...) {
                        // Ignore parsing errors, continue reading
                    }
                }
            }
        }
    }

    return request;
}
