#include "Server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <array>
#include <algorithm>
#include <cerrno>
#include <system_error>
#include <sstream>
#include <fstream>

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
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        // Handle client in a separate thread or process would be better,
        // but for simplicity we'll handle it sequentially
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

    // Extract the path from the HTTP request
    size_t start = request.find("GET ") + 4;
    size_t end = request.find(" HTTP/");
    if (start == std::string::npos || end == std::string::npos) {
        send404(clientSock);
        return;
    }

    std::string path = request.substr(start, end - start);
    if (path == "/" || path.empty()) {
        path = "/index.html";
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/audio") {
        sendAudio(clientSock);
    } else {
    // Serve static files from public directory
    std::string filePath = "../public" + path;
    std::string mimeType = getMimeType(filePath);

    // Debug output
    std::cout << "Requesting path: " << path << ", filePath: " << filePath << std::endl;

    // Check if file exists
    if (access(filePath.c_str(), F_OK) != -1) {
        sendFile(clientSock, filePath, mimeType);
    } else {
        std::cout << "File not found: " << filePath << std::endl;
        send404(clientSock);
    }
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

    // Prepare HTTP response header
    std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + mimeType + "\r\n"
        "Content-Length: " + std::to_string(fileSize) + "\r\n"
        "Connection: close\r\n"
        "\r\n";

    if (!sendAll(clientSock, header.c_str(), header.size())) {
        return;
    }

    // Send file content in chunks
    std::array<char, 65536> fileBuffer{};
    while (file) {
        file.read(fileBuffer.data(), fileBuffer.size());
        size_t bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (!sendAll(clientSock, fileBuffer.data(), bytesRead)) {
                return;
            }
        }
    }
}

void Server::sendAudio(int clientSock) {
    std::string audioPath = "../data/track.wav";
    std::ifstream file(audioPath, std::ios::binary);
    if (!file) {
        send404(clientSock);
        return;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Prepare HTTP response header
    std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: audio/wav\r\n"
        "Content-Length: " + std::to_string(fileSize) + "\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "\r\n";

    if (!sendAll(clientSock, header.c_str(), header.size())) {
        return;
    }

    // Send audio file content in chunks
    std::array<char, 65536> audioBuffer{};
    while (file) {
        file.read(audioBuffer.data(), audioBuffer.size());
        size_t bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (!sendAll(clientSock, audioBuffer.data(), bytesRead)) {
                return;
            }
        }
    }
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
    if (endsWith(path, ".wav")) return "audio/wav";
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
