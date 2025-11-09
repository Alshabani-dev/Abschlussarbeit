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
    // Ignore SIGPIPE to prevent crashes on broken connections
    signal(SIGPIPE, SIG_IGN);
}

void Server::run() {
    int server_fd, client_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket
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

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        // Accept connection
        if ((client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        handleClient(client_sock);
        close(client_sock);
    }
}

void Server::handleClient(int clientSock) {
    std::string request = readRequest(clientSock);
    if (request.empty()) {
        return;
    }

    HttpRequest httpRequest;
    httpRequest.parse(request);

    HttpResponse httpResponse;

    // Route requests
    if (httpRequest.getMethod() == "GET") {
        FileHandler fileHandler;
        std::string path = httpRequest.getPath();
        // If path is root, serve index.html
        if (path == "/") {
            path = "/index.html";
        }
        std::string fileContent = fileHandler.handle(path);

        if (!fileContent.empty()) {
            // Get file extension for MIME type
            size_t dotPos = path.find_last_of('.');
            std::string ext = (dotPos != std::string::npos) ?
                path.substr(dotPos) : "";

            httpResponse.setStatus(200, "OK");
            httpResponse.setHeader("Content-Type", fileHandler.getMimeType(ext));
            httpResponse.setBody(fileContent);
        } else {
            httpResponse.setStatus(404, "Not Found");
            httpResponse.setBody("<html><body><h1>404 Not Found</h1></body></html>");
        }
    } else if (httpRequest.getMethod() == "POST") {
        try {
            UploadHandler uploadHandler;
            // Get the body directly as vector<char>
            std::vector<char> bodyVec = httpRequest.getBody();
            std::string result = uploadHandler.handle(
                bodyVec,
                httpRequest.getHeader("Content-Type")
            );

            if (result.find("200 OK") != std::string::npos) {
                httpResponse.setStatus(200, "OK");
                httpResponse.setBody("<html><body><h1>File uploaded successfully</h1></body></html>");
            } else if (result.find("400 Bad Request") != std::string::npos) {
                httpResponse.setStatus(400, "Bad Request");
                std::string errorMsg = result.substr(result.find(": ") + 2);
                httpResponse.setBody("<html><body><h1>400 Bad Request: " + errorMsg + "</h1></body></html>");
            } else {
                httpResponse.setStatus(500, "Internal Server Error");
                std::string errorMsg = result.substr(result.find(": ") + 2);
                httpResponse.setBody("<html><body><h1>500 Internal Server Error: " + errorMsg + "</h1></body></html>");
            }
        } catch (const std::exception& e) {
            httpResponse.setStatus(500, "Internal Server Error");
            httpResponse.setBody("<html><body><h1>500 Internal Server Error: " + std::string(e.what()) + "</h1></body></html>");
        } catch (...) {
            httpResponse.setStatus(500, "Internal Server Error");
            httpResponse.setBody("<html><body><h1>500 Internal Server Error: Unknown error</h1></body></html>");
        }
    } else {
        httpResponse.setStatus(405, "Method Not Allowed");
        httpResponse.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
    }

    // Send response
    std::string responseStr = httpResponse.toString();
    send(clientSock, responseStr.c_str(), responseStr.size(), 0);
}

std::string Server::readRequest(int clientSock) {
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    std::string request;

    // Read headers first
    while (true) {
        ssize_t bytesRead = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead <= 0) {
            break;
        }

        buffer[bytesRead] = '\0';
        request += buffer;

        // Check if we have the full headers
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            // Check if there's a body
            std::string contentLengthStr = "Content-Length: ";
            size_t contentLengthPos = request.find(contentLengthStr);
            if (contentLengthPos != std::string::npos) {
                size_t contentLengthEnd = request.find("\r\n", contentLengthPos);
                if (contentLengthEnd != std::string::npos) {
                    int contentLength = std::stoi(
                        request.substr(contentLengthPos + contentLengthStr.length(),
                                        contentLengthEnd - (contentLengthPos + contentLengthStr.length()))
                    );

                    // Read the rest of the body
                    while (request.size() - (headerEnd + 4) < contentLength) {
                        bytesRead = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
                        if (bytesRead <= 0) {
                            break;
                        }
                        buffer[bytesRead] = '\0';
                        request += buffer;
                    }
                }
            }
            break;
        }
    }

    return request;
}
