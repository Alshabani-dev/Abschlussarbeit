#include "Server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <csignal>
#include <cerrno>

Server::Server(int port) : port_(port), serverSock_(-1), running_(false) {
    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to prevent crashes
}

Server::~Server() {
    if (serverSock_ != -1) {
        close(serverSock_);
    }
}

void Server::setupSocket() {
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSock_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSock_, 10) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::run() {
    running_ = true;
    setupSocket();
    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSock < 0) {
            if (errno == EINTR) continue; // Interrupted system call
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            continue;
        }

        // Handle client in a separate thread or process would be better,
        // but for simplicity we'll handle it sequentially
        handleClient(clientSock);
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    char buffer[4096];
    ssize_t bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer);

    // Extract the path from the HTTP request
    size_t start = request.find("GET ") + 4;
    size_t end = request.find(" HTTP/1.");
    if (start == std::string::npos || end == std::string::npos) {
        send404(clientSock);
        return;
    }

    std::string path = request.substr(start, end - start);
    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/") {
        sendFile(clientSock, "public/index.html", "text/html");
    } else if (path == "/app.js") {
        sendFile(clientSock, "public/app.js", "application/javascript");
    } else if (path == "/styles.css") {
        sendFile(clientSock, "public/styles.css", "text/css");
    } else if (path == "/audio") {
        sendAudio(clientSock);
    } else {
        send404(clientSock);
    }
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        send404(clientSock);
        return;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Prepare HTTP headers
    std::ostringstream headers;
    headers << "HTTP/1.1 200 OK\r\n";
    headers << "Content-Type: " << mimeType << "\r\n";
    headers << "Content-Length: " << fileSize << "\r\n";
    headers << "Connection: close\r\n\r\n";

    // Send headers
    std::string headerStr = headers.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }

    // Send file content in chunks
    const size_t bufferSize = 65536; // 64KB chunks
    char buffer[bufferSize];

    while (file) {
        file.read(buffer, bufferSize);
        size_t bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (!sendAll(clientSock, buffer, bytesRead)) {
                return;
            }
        }
    }
}

void Server::sendAudio(int clientSock) {
    const std::string audioPath = "data/track.wav";
    int fd = open(audioPath.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    // Get file size
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        send500(clientSock, "Failed to get file stats");
        return;
    }

    // Prepare HTTP headers for audio streaming
    std::ostringstream headers;
    headers << "HTTP/1.1 200 OK\r\n";
    headers << "Content-Type: audio/wav\r\n";
    headers << "Content-Length: " << fileStat.st_size << "\r\n";
    headers << "Cache-Control: no-store\r\n";
    headers << "Connection: close\r\n\r\n";

    // Send headers
    std::string headerStr = headers.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        close(fd);
        return;
    }

    // Stream file in 64KB chunks
    char buffer[65536];

    while (true) {
        ssize_t bytesRead = read(fd, buffer, 65536);
        if (bytesRead <= 0) break;

        if (!sendAll(clientSock, buffer, bytesRead)) {
            break;
        }
    }

    close(fd);
}

void Server::send404(int clientSock) {
    std::string response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n\r\n"
        "404 Not Found";

    sendAll(clientSock, response.c_str(), response.size());
}

void Server::send500(int clientSock, const std::string& message) {
    std::string response =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n\r\n"
        "500 Internal Server Error: " + message;

    sendAll(clientSock, response.c_str(), response.size());
}

// C++17 compatible endsWith function
bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::string Server::getMimeType(const std::string& path) const {
    if (endsWith(path, ".html")) return "text/html";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    return "application/octet-stream";
}

bool Server::sendAll(int sock, const char* data, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
        ssize_t sent = send(sock, data + totalSent, length - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
            return false;
        }
        totalSent += sent;
    }
    return true;
}
