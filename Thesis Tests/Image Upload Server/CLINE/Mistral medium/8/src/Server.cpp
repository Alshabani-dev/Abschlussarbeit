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
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>
#include <algorithm>

Server::Server(int port) : port_(port) {}

void Server::run() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        throw std::runtime_error("Socket creation failed");
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw std::runtime_error("Setsockopt failed");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Bind failed");
    }

    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        throw std::runtime_error("Listen failed");
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        // Accept incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        // Handle client in a separate function
        handleClient(new_socket);

        // Close the connection
        close(new_socket);
    }
}

void Server::handleClient(int clientSock) {
    try {
        std::string rawRequest = readRequest(clientSock);

        // Parse the request
        HttpRequest request;
        request.parse(rawRequest);

        // Create response object
        HttpResponse response;

        // Route the request
        if (request.getMethod() == "GET") {
            // Handle GET requests (static files)
            FileHandler fileHandler;
            std::string fileContent = fileHandler.handle(request.getPath());

            if (!fileContent.empty()) {
                // File found, send it
                response.setStatus(200, "OK");
                response.setHeader("Content-Type", fileHandler.getMimeType(request.getPath()));
                response.setBody(fileContent);
            } else {
                // File not found
                response.setStatus(404, "Not Found");
                response.setBody("<h1>404 Not Found</h1><p>The requested resource was not found.</p>");
            }
        } else if (request.getMethod() == "POST") {
            // Handle POST requests (file uploads)
            UploadHandler uploadHandler;
            std::string uploadResult = uploadHandler.handle(
                request.getBody(),
                request.getHeader("Content-Type")
            );

            // Parse the result to get status code
            int statusCode = 200;
            std::string statusText = "OK";
            std::string responseBody = "File uploaded successfully";

            if (uploadResult.find("400") != std::string::npos) {
                statusCode = 400;
                statusText = "Bad Request";
                responseBody = uploadResult.substr(uploadResult.find(":") + 1);
            } else if (uploadResult.find("500") != std::string::npos) {
                statusCode = 500;
                statusText = "Internal Server Error";
                responseBody = uploadResult.substr(uploadResult.find(":") + 1);
            }

            response.setStatus(statusCode, statusText);
            response.setBody(responseBody);
        } else {
            // Unsupported method
            response.setStatus(405, "Method Not Allowed");
            response.setBody("<h1>405 Method Not Allowed</h1><p>Only GET and POST methods are supported.</p>");
        }

        // Send the response
        std::string responseStr = response.toString();
        send(clientSock, responseStr.c_str(), responseStr.size(), 0);
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;

        // Send error response
        std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n"
                                    "Content-Type: text/plain\r\n"
                                    "Connection: close\r\n"
                                    "\r\n"
                                    "Internal Server Error";
        send(clientSock, errorResponse.c_str(), errorResponse.size(), 0);
    }
}

std::string Server::readRequest(int clientSock) {
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE] = {0};
    std::vector<char> requestData;

    // Read data in chunks until we get the full request
    while (true) {
        ssize_t bytesRead = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);

        if (bytesRead <= 0) {
            break;
        }

        // Append to our binary-safe buffer
        requestData.insert(requestData.end(), buffer, buffer + bytesRead);

        // Convert to string for header parsing
        std::string requestStr(requestData.begin(), requestData.end());

        // Check if we have the full request (Content-Length header)
        size_t contentLengthPos = requestStr.find("Content-Length:");

        if (contentLengthPos != std::string::npos) {
            size_t endOfHeader = requestStr.find("\r\n\r\n", contentLengthPos);
            if (endOfHeader != std::string::npos) {
                // Extract content length
                size_t contentLengthStart = contentLengthPos + 15; // "Content-Length: "
                size_t contentLengthEnd = requestStr.find("\r\n", contentLengthStart);
                int contentLength = std::stoi(requestStr.substr(contentLengthStart, contentLengthEnd - contentLengthStart));

                // Check if we have the full body
                size_t bodyStart = endOfHeader + 4;
                if (requestData.size() >= bodyStart + contentLength) {
                    break;
                }
            }
        } else {
            // For requests without Content-Length (like GET), check for end of headers
            if (requestStr.find("\r\n\r\n") != std::string::npos) {
                break;
            }
        }
    }

    return std::string(requestData.begin(), requestData.end());
}
