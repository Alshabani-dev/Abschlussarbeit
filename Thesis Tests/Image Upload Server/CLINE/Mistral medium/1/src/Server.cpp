#include "Server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/sendfile.h>

Server::Server(int port) : port_(port) {}

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

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        handleClient(new_socket);
        close(new_socket);
    }
}

void Server::handleClient(int clientSock) {
    std::vector<char> requestData = readRequest(clientSock);

    // Create HttpRequest object
    HttpRequest request(requestData);

    // Process request
    HttpResponse response = processRequest(request);

    // Send response
    std::vector<char> responseBytes = response.toBytes();
    send(clientSock, responseBytes.data(), responseBytes.size(), MSG_NOSIGNAL);
}

std::vector<char> Server::readRequest(int clientSock) {
    std::vector<char> buffer(1024 * 1024); // 1MB buffer
    ssize_t bytes_read;
    std::vector<char> requestData;

    // Read content length first to know how much to read
    bool hasContentLength = false;
    size_t contentLength = 0;
    size_t headerEnd = 0;

    // First read headers to get Content-Length
    while ((bytes_read = read(clientSock, buffer.data(), buffer.size())) > 0) {
        requestData.insert(requestData.end(), buffer.begin(), buffer.begin() + bytes_read);

        // Check if we have the headers
        if (!hasContentLength) {
            std::string requestStr(requestData.begin(), requestData.end());
            size_t contentLengthPos = requestStr.find("Content-Length:");

            if (contentLengthPos != std::string::npos) {
                size_t endPos = requestStr.find("\r\n", contentLengthPos);
                if (endPos == std::string::npos) {
                    endPos = requestStr.find("\n", contentLengthPos);
                }

                if (endPos != std::string::npos) {
                    std::string contentLengthStr = requestStr.substr(
                        contentLengthPos + 15,
                        endPos - (contentLengthPos + 15)
                    );
                    contentLength = std::stoul(contentLengthStr);
                    hasContentLength = true;
                }
            }

            // Find end of headers
            size_t doubleNewlinePos = requestStr.find("\r\n\r\n");
            if (doubleNewlinePos == std::string::npos) {
                doubleNewlinePos = requestStr.find("\n\n");
            }

            if (doubleNewlinePos != std::string::npos) {
                headerEnd = doubleNewlinePos + (requestStr[doubleNewlinePos+1] == '\n' ? 4 : 2);
                break;
            }
        }
    }

    // If we have content length, read the remaining body
    if (hasContentLength && requestData.size() < headerEnd + contentLength) {
        size_t remaining = headerEnd + contentLength - requestData.size();
        while (remaining > 0 && (bytes_read = read(clientSock, buffer.data(), std::min(buffer.size(), remaining))) > 0) {
            requestData.insert(requestData.end(), buffer.begin(), buffer.begin() + bytes_read);
            remaining -= bytes_read;
        }
    }

    return requestData;
}

HttpResponse Server::processRequest(const HttpRequest& request) {
    std::string method = request.getMethod();
    std::string path = request.getPath();

    // Handle GET requests for static files
    if (method == "GET") {
        if (path == "/" || path.empty()) {
            path = "/index.html";
        }

        std::string filePath = "public" + path;
        if (fileHandler_.fileExists(filePath)) {
            std::vector<char> fileContent = fileHandler_.readFile(filePath);
            std::string mimeType = fileHandler_.getMimeType(filePath);

            HttpResponse response(200, mimeType);
            response.setBody(fileContent);
            return response;
        } else {
            HttpResponse response(404, "text/html");
            response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
            return response;
        }
    }
    // Handle POST requests for file uploads
    else if (method == "POST") {
        std::string contentType = request.getContentType();
        std::vector<char> body = request.getBody();

        std::string result = uploadHandler_.handle(body, contentType);

        HttpResponse response(200, "text/plain");
        response.setBody(result);
        return response;
    }
    // Method not allowed
    else {
        HttpResponse response(405, "text/html");
        response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
        return response;
    }
}
