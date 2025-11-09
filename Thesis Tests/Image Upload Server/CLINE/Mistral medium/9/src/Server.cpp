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
#include <csignal>
#include <vector>

Server::Server(int port) : port_(port) {
    // Ignore SIGPIPE to prevent crashes on client disconnect
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

    // Main server loop
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
    std::string request = readRequest(clientSock);

    HttpRequest httpRequest;
    httpRequest.parse(request);

    HttpResponse httpResponse;

    // Route requests
    if (httpRequest.getMethod() == "GET" && httpRequest.getPath() == "/") {
        FileHandler fileHandler;
        std::string response = fileHandler.handle("/index.html");
        httpResponse.setStatus(200);
        httpResponse.setHeader("Content-Type", "text/html");
        httpResponse.setBody(response);
    }
    else if (httpRequest.getMethod() == "GET" && httpRequest.getPath() == "/styles.css") {
        FileHandler fileHandler;
        std::string response = fileHandler.handle("/styles.css");
        httpResponse.setStatus(200);
        httpResponse.setHeader("Content-Type", "text/css");
        httpResponse.setBody(response);
    }
    else if (httpRequest.getMethod() == "GET" && httpRequest.getPath() == "/scripts.js") {
        FileHandler fileHandler;
        std::string response = fileHandler.handle("/scripts.js");
        httpResponse.setStatus(200);
        httpResponse.setHeader("Content-Type", "application/javascript");
        httpResponse.setBody(response);
    }
    else if (httpRequest.getMethod() == "POST" && httpRequest.getPath() == "/") {
        UploadHandler uploadHandler;
        std::string response = uploadHandler.handle(
            httpRequest.getBody(),
            httpRequest.getHeader("Content-Type")
        );
        httpResponse.setStatus(200);
        httpResponse.setHeader("Content-Type", "text/plain");
        httpResponse.setBody(response);
    }
    else {
        httpResponse.setStatus(404);
        httpResponse.setHeader("Content-Type", "text/plain");
        httpResponse.setBody("404 Not Found");
    }

    // Send response
    std::string responseStr = httpResponse.build();
    send(clientSock, responseStr.c_str(), responseStr.size(), 0);
}

std::string Server::readRequest(int clientSock) {
    std::vector<char> buffer(1024);
    std::string request;

    while (true) {
        ssize_t bytesRead = recv(clientSock, buffer.data(), buffer.size(), 0);
        if (bytesRead <= 0) break;

        request.append(buffer.data(), bytesRead);

        // Check for end of headers (double CRLF)
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            // Check if we have the full body based on Content-Length
            size_t contentLengthPos = request.find("Content-Length: ");
            if (contentLengthPos != std::string::npos) {
                size_t contentLengthEnd = request.find("\r\n", contentLengthPos);
                std::string contentLengthStr = request.substr(
                    contentLengthPos + 16,
                    contentLengthEnd - (contentLengthPos + 16)
                );

                int contentLength = std::stoi(contentLengthStr);
                size_t bodyStart = headerEnd + 4;

                // If we have the full body, we're done
                if (request.length() >= bodyStart + contentLength) {
                    break;
                }
            } else {
                // No Content-Length, assume we have the full request
                break;
            }
        }
    }

    return request;
}
