#include "Server.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <cerrno>
#include <array>

Server::Server(int port) : port_(port), serverSock_(-1), running_(false) {
    // Ignore SIGPIPE to prevent crashes on broken pipe
    signal(SIGPIPE, SIG_IGN);
    setupSocket();
}

Server::~Server() {
    running_ = false;
    if (serverSock_ >= 0) {
        close(serverSock_);
    }
}

void Server::setupSocket() {
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Allow socket reuse for quick restart
    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverSock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to bind socket on port " + std::to_string(port_));
    }

    if (listen(serverSock_, 10) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "🚀 Server listening on http://localhost:" << port_ << std::endl;
}

void Server::run() {
    running_ = true;
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSock < 0) {
            if (errno == EINTR) continue;
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
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

    // Parse HTTP request line
    size_t methodEnd = request.find(' ');
    if (methodEnd == std::string::npos) return;
    
    std::string method = request.substr(0, methodEnd);
    size_t pathEnd = request.find(' ', methodEnd + 1);
    if (pathEnd == std::string::npos) return;
    
    std::string path = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);

    if (method == "GET") {
        handleGet(path, clientSock);
    }
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/" || path == "/index.html") {
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

    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        send404(clientSock);
        return;
    }

    std::string header = "HTTP/1.1 200 OK\r\n";
    header += "Content-Type: " + mimeType + "\r\n";
    header += "Content-Length: " + std::to_string(fileStat.st_size) + "\r\n";
    header += "Connection: close\r\n";
    header += "\r\n";

    if (!sendAll(clientSock, header.c_str(), header.length())) {
        close(fd);
        return;
    }

    std::array<char, 65536> buffer{};
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer.data(), buffer.size())) > 0) {
        if (!sendAll(clientSock, buffer.data(), bytesRead)) {
            break;
        }
    }

    close(fd);
}

void Server::sendAudio(int clientSock) {
    const std::string audioPath = "data/track.wav";
    int fd = open(audioPath.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        send500(clientSock, "Failed to stat audio file");
        return;
    }

    // Send HTTP headers for audio streaming
    std::string header = "HTTP/1.1 200 OK\r\n";
    header += "Content-Type: audio/wav\r\n";
    header += "Content-Length: " + std::to_string(fileStat.st_size) + "\r\n";
    header += "Cache-Control: no-store\r\n";
    header += "Connection: close\r\n";
    header += "\r\n";

    if (!sendAll(clientSock, header.c_str(), header.length())) {
        close(fd);
        return;
    }

    // Binary-safe streaming with 64KB chunks
    std::array<char, 65536> buffer{};
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer.data(), buffer.size())) > 0) {
        if (!sendAll(clientSock, buffer.data(), bytesRead)) {
            break;
        }
    }

    close(fd);
}

void Server::send404(int clientSock) {
    std::string response = "HTTP/1.1 404 Not Found\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: 9\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += "Not Found";
    
    sendAll(clientSock, response.c_str(), response.length());
}

void Server::send500(int clientSock, const std::string& message) {
    std::string response = "HTTP/1.1 500 Internal Server Error\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Content-Length: " + std::to_string(message.length()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += message;
    
    sendAll(clientSock, response.c_str(), response.length());
}

std::string Server::getMimeType(const std::string& path) const {
    // C++17-compatible endsWith lambda
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
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return false;
        }
        totalSent += sent;
    }
    return true;
}
