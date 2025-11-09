#include "Server.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <fstream>

namespace {
constexpr size_t kRequestBufferSize = 4096;
constexpr size_t kStreamChunkSize = 64 * 1024;
}

Server::Server(int port) : port_(port), serverSock_(-1), running_(false) {
    signal(SIGPIPE, SIG_IGN);
    setupSocket();
}

Server::~Server() {
    if (serverSock_ >= 0) {
        close(serverSock_);
    }
}

void Server::setupSocket() {
    serverSock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int enable = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSock_, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::run() {
    running_ = true;
    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSock < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("accept");
            continue;
        }

        handleClient(clientSock);
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    std::array<char, kRequestBufferSize> buffer{};
    ssize_t bytesReceived = recv(clientSock, buffer.data(), buffer.size(), 0);
    if (bytesReceived <= 0) {
        return;
    }

    std::string request(buffer.data(), static_cast<size_t>(bytesReceived));
    std::istringstream requestStream(request);
    std::string method;
    std::string path;
    std::string version;
    requestStream >> method >> path >> version;

    if (method != "GET") {
        send404(clientSock);
        return;
    }

    handleGet(path, clientSock);
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
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        send404(clientSock);
        return;
    }

    std::ostringstream bodyStream;
    bodyStream << file.rdbuf();
    const std::string body = bodyStream.str();

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << mimeType << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "Connection: close\r\n\r\n";

    const std::string headers = response.str();
    if (!sendAll(clientSock, headers.c_str(), headers.size())) {
        return;
    }
    sendAll(clientSock, body.c_str(), body.size());
}

void Server::sendAudio(int clientSock) {
    const std::string filePath = "data/track.wav";
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    struct stat st {};
    if (fstat(fd, &st) < 0) {
        close(fd);
        send500(clientSock, "Unable to stat WAV file");
        return;
    }

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: audio/wav\r\n"
             << "Content-Length: " << st.st_size << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "Connection: close\r\n\r\n";

    const std::string headers = response.str();
    if (!sendAll(clientSock, headers.c_str(), headers.size())) {
        close(fd);
        return;
    }

    std::array<char, kStreamChunkSize> buffer{};
    ssize_t bytesRead = 0;
    while ((bytesRead = read(fd, buffer.data(), buffer.size())) > 0) {
        if (!sendAll(clientSock, buffer.data(), static_cast<size_t>(bytesRead))) {
            break;
        }
    }

    if (bytesRead < 0) {
        std::perror("read");
    }

    close(fd);
}

void Server::send404(int clientSock) {
    const std::string body = "<h1>404 Not Found</h1>";
    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;
    const std::string payload = response.str();
    sendAll(clientSock, payload.c_str(), payload.size());
}

void Server::send500(int clientSock, const std::string& message) {
    std::string body = "<h1>500 Internal Server Error</h1><p>" + message + "</p>";
    std::ostringstream response;
    response << "HTTP/1.1 500 Internal Server Error\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;
    const std::string payload = response.str();
    sendAll(clientSock, payload.c_str(), payload.size());
}

std::string Server::getMimeType(const std::string& path) const {
    auto endsWith = [](const std::string& str, const std::string& suffix) {
        if (suffix.size() > str.size()) {
            return false;
        }
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    if (endsWith(path, ".html")) return "text/html";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    if (endsWith(path, ".png")) return "image/png";
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) return "image/jpeg";
    if (endsWith(path, ".svg")) return "image/svg+xml";
    if (endsWith(path, ".ico")) return "image/x-icon";
    if (endsWith(path, ".json")) return "application/json";
    if (endsWith(path, ".txt")) return "text/plain";
    return "application/octet-stream";
}

bool Server::sendAll(int sock, const char* data, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
        ssize_t sent = send(sock, data + totalSent, length - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            std::perror("send");
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}
