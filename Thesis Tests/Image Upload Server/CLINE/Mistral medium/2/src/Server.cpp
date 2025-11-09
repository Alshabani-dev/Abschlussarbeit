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
#include <csignal>

Server::Server(int port) : port_(port) {
    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to prevent crashes on client disconnect
}

void Server::run() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        // Accept incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // Handle client
        handleClient(new_socket);
        close(new_socket);
    }
}

void Server::handleClient(int clientSock) {
    std::string request = readRequest(clientSock);
    std::cout << "Received request: " << request << std::endl;

    HttpRequest httpRequest(request);
    std::string method = httpRequest.getMethod();
    std::string path = httpRequest.getPath();
    std::string responseBody;
    std::string contentType;
    int statusCode = 200;
    std::string statusText = "OK";

    if (method == "GET") {
        if (path == "/") {
            FileHandler fileHandler;
            responseBody = fileHandler.readFile("public/index.html");
            contentType = fileHandler.getMimeType("public/index.html");
        } else if (path == "/styles.css") {
            FileHandler fileHandler;
            responseBody = fileHandler.readFile("public/styles.css");
            contentType = fileHandler.getMimeType("public/styles.css");
        } else if (path == "/scripts.js") {
            FileHandler fileHandler;
            responseBody = fileHandler.readFile("public/scripts.js");
            contentType = fileHandler.getMimeType("public/scripts.js");
        } else {
            statusCode = 404;
            statusText = "Not Found";
            responseBody = "<h1>404 Not Found</h1>";
            contentType = "text/html";
        }
    } else if (method == "POST") {
        UploadHandler uploadHandler;
        std::string contentTypeHeader = httpRequest.getHeader("Content-Type");
        std::string uploadResponse = uploadHandler.handle(httpRequest.getBody(), contentTypeHeader);

        if (uploadResponse.find("200 OK") != std::string::npos) {
            responseBody = "<script>alert('File uploaded successfully!');</script>";
            contentType = "text/html";
        } else {
            statusCode = 400;
            statusText = "Bad Request";
            responseBody = "<script>alert('" + uploadResponse + "');</script>";
            contentType = "text/html";
        }
    } else {
        statusCode = 405;
        statusText = "Method Not Allowed";
        responseBody = "<h1>405 Method Not Allowed</h1>";
        contentType = "text/html";
    }

    HttpResponse httpResponse(statusCode, statusText, contentType, responseBody);
    std::string response = httpResponse.toString();
    send(clientSock, response.c_str(), response.size(), 0);
}

std::string Server::readRequest(int clientSock) {
    char buffer[1024] = {0};
    std::string request;
    ssize_t bytesRead;

    // Read headers first
    while ((bytesRead = read(clientSock, buffer, sizeof(buffer))) > 0) {
        request.append(buffer, bytesRead);
        if (request.find("\r\n\r\n") != std::string::npos || request.find("\n\n") != std::string::npos) {
            break; // End of headers
        }
    }

    // Check for Content-Length
    size_t contentLengthPos = request.find("Content-Length: ");
    if (contentLengthPos != std::string::npos) {
        size_t endOfLine = request.find("\r\n", contentLengthPos);
        std::string contentLengthStr = request.substr(contentLengthPos + 16, endOfLine - (contentLengthPos + 16));
        int contentLength = std::stoi(contentLengthStr);

        // Calculate body length already read
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart == std::string::npos) {
            bodyStart = request.find("\n\n");
        }
        if (bodyStart != std::string::npos) {
            bodyStart += 4;
            size_t bodyRead = request.size() - bodyStart;
            size_t remaining = contentLength - bodyRead;

            // Read remaining body
            while (remaining > 0) {
                bytesRead = read(clientSock, buffer, std::min(remaining, sizeof(buffer)));
                if (bytesRead <= 0) break;
                request.append(buffer, bytesRead);
                remaining -= bytesRead;
            }
        }
    }

    return request;
}
