#include "Server.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "FileHandler.h"
#include "UploadHandler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <fcntl.h>

Server::Server(int port) : port_(port) {}

void Server::run() {
    int server_fd, client_socket;
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
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // Handle client in a separate function
        handleClient(client_socket);
        close(client_socket);
    }
}

void Server::handleClient(int clientSock) {
    std::string request = readRequest(clientSock);

    HttpRequest httpRequest;
    httpRequest.parse(request);

    HttpResponse response;

    if (httpRequest.getMethod() == "GET") {
        FileHandler fileHandler;
        std::string path = httpRequest.getPath();

        // Default to index.html for root path
        if (path == "/") {
            path = "/index.html";
        }

        std::string content = fileHandler.handle(path);

        // Determine MIME type based on file extension
        size_t dotPos = path.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string extension = path.substr(dotPos);
            std::string mimeType = fileHandler.getMimeType(extension);
            response.setHeader("Content-Type", mimeType);
        } else {
            response.setHeader("Content-Type", "text/plain");
        }

        response.setStatus(200);
        response.setBody(content);
    }
    else if (httpRequest.getMethod() == "POST" && httpRequest.getPath() == "/") {
        UploadHandler uploadHandler;
        // Get the content type header
        std::string contentType = httpRequest.getHeader("Content-Type");

        // Handle the upload with the body data
        std::string result = uploadHandler.handle(httpRequest.getBody(), contentType);

        // Check if the result contains an error code
        if (result.find("200 OK") != std::string::npos) {
            response.setStatus(200);
            response.setHeader("Content-Type", "text/plain");
            response.setBody("File uploaded successfully");
        } else if (result.find("400 Bad Request") != std::string::npos) {
            response.setStatus(400);
            response.setHeader("Content-Type", "text/plain");
            response.setBody(result);
        } else if (result.find("500 Internal Server Error") != std::string::npos) {
            response.setStatus(500);
            response.setHeader("Content-Type", "text/plain");
            response.setBody(result);
        } else {
            response.setStatus(200);
            response.setHeader("Content-Type", "text/plain");
            response.setBody(result);
        }
    }
    else {
        response.setStatus(404);
        response.setHeader("Content-Type", "text/plain");
        response.setBody("404 Not Found");
    }

    // Send response
    std::string responseStr = response.toString();
    send(clientSock, responseStr.c_str(), responseStr.size(), 0);
}

std::string Server::readRequest(int clientSock) {
    std::vector<char> buffer(1024);
    std::vector<char> requestData;
    ssize_t bytesRead;

    // Read content length if available
    size_t contentLength = 0;
    std::string headerBuffer;
    bool headersComplete = false;

    // First read headers to get content length
    while ((bytesRead = recv(clientSock, buffer.data(), buffer.size(), 0)) > 0) {
        std::string chunk(buffer.data(), bytesRead);
        headerBuffer += chunk;

        // Check if headers are complete
        size_t headerEnd = headerBuffer.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            headersComplete = true;

            // Parse Content-Length header
            size_t contentLengthPos = headerBuffer.find("Content-Length: ");
            if (contentLengthPos != std::string::npos) {
                size_t endPos = headerBuffer.find("\r\n", contentLengthPos);
                std::string contentLengthStr = headerBuffer.substr(
                    contentLengthPos + 16,
                    endPos - (contentLengthPos + 16)
                );
                contentLength = std::stoul(contentLengthStr);
            }

            // Add the remaining part of the header to request data
            requestData.insert(requestData.end(), headerBuffer.begin(), headerBuffer.begin() + headerEnd + 4);

            // Remove headers from buffer
            headerBuffer.erase(0, headerEnd + 4);

            // If there's body data in the header buffer, add it to request data
            if (!headerBuffer.empty()) {
                requestData.insert(requestData.end(), headerBuffer.begin(), headerBuffer.end());
            }

            break;
        }
    }

    // If we have a content length, read the remaining body
    if (headersComplete && contentLength > 0) {
        size_t currentSize = requestData.size();
        size_t remaining = contentLength - (currentSize - (headerBuffer.find("\r\n\r\n") + 4));

        while (remaining > 0) {
            bytesRead = recv(clientSock, buffer.data(), std::min(buffer.size(), static_cast<size_t>(remaining)), 0);
            if (bytesRead <= 0) break;

            requestData.insert(requestData.end(), buffer.begin(), buffer.begin() + bytesRead);
            remaining -= bytesRead;
        }
    }

    // Convert to string for processing
    return std::string(requestData.begin(), requestData.end());
}
