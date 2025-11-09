#include "Server.h"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <csignal>
#include <cerrno>
#include <array>
#include <algorithm>

Server::Server(int port) : port_(port), serverSock_(-1), running_(false) {
    signal(SIGPIPE, SIG_IGN);
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
    std::cout << "Server running on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSock < 0) {
            if (errno == EINTR) continue;
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
        }

        handleClient(clientSock);
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    std::array<char, 4096> buffer{};
    ssize_t bytesRead = recv(clientSock, buffer.data(), buffer.size() - 1, 0);

    if (bytesRead <= 0) {
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer.data());

    size_t methodEnd = request.find(' ');
    if (methodEnd == std::string::npos) {
        send404(clientSock);
        return;
    }

    size_t pathEnd = request.find(' ', methodEnd + 1);
    if (pathEnd == std::string::npos) {
        send404(clientSock);
        return;
    }

    std::string path = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);
    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/") {
        sendFile(clientSock, "public/index.html", "text/html");
    } else if (path == "/styles.css") {
        sendFile(clientSock, "public/styles.css", "text/css");
    } else if (path == "/app.js") {
        sendFile(clientSock, "public/app.js", "application/javascript");
    } else if (path == "/audio") {
        sendAudio(clientSock);
    } else {
        send404(clientSock);
    }
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    struct stat fileStat{};
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        send500(clientSock, "Failed to get file stats");
        return;
    }

    std::string headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + mimeType + "\r\n"
        "Content-Length: " + std::to_string(fileStat.st_size) + "\r\n"
        "Connection: close\r\n"
        "\r\n";

    if (!sendAll(clientSock, headers.c_str(), headers.size())) {
        close(fd);
        return;
    }

    std::array<char, 65536> fileBuffer{};
    ssize_t bytesRead;
    while ((bytesRead = read(fd, fileBuffer.data(), fileBuffer.size())) > 0) {
        if (!sendAll(clientSock, fileBuffer.data(), bytesRead)) {
            break;
        }
    }

    close(fd);
}

void Server::sendAudio(int clientSock) {
    int fd = open("data/track.wav", O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    struct stat fileStat{};
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        send500(clientSock, "Failed to get file stats");
        return;
    }

    std::string headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: audio/wav\r\n"
        "Content-Length: " + std::to_string(fileStat.st_size) + "\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "\r\n";

    if (!sendAll(clientSock, headers.c_str(), headers.size())) {
        close(fd);
        return;
    }

    std::array<char, 65536> fileBuffer{};
    ssize_t bytesRead;
    while ((bytesRead = read(fd, fileBuffer.data(), fileBuffer.size())) > 0) {
        if (!sendAll(clientSock, fileBuffer.data(), bytesRead)) {
            break;
        }
    }

    close(fd);
}

void Server::send404(int clientSock) {
    std::string response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "404 Not Found";

    sendAll(clientSock, response.c_str(), response.size());
}

void Server::send500(int clientSock, const std::string& message) {
    std::string response =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "500 Internal Server Error: " + message;

    sendAll(clientSock, response.c_str(), response.size());
}

std::string Server::getMimeType(const std::string& path) const {
    auto endsWith = [](const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    };

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
