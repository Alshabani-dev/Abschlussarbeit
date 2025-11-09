#include "Server.h"

#include <arpa/inet.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <array>

namespace {
constexpr int kBacklog = 16;
constexpr size_t kChunkSize = 65536;

std::string sanitizePath(std::string path) {
    auto queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);
    }
    if (path.empty()) {
        return "/";
    }
    return path;
}
}  // namespace

Server::Server(int port) : port_(port), serverSock_(-1), running_(false) {
    std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSock_ >= 0) {
        ::close(serverSock_);
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

    if (listen(serverSock_, kBacklog) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::run() {
    setupSocket();
    running_ = true;

    std::cout << "Server listening on port " << port_ << std::endl;
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSock < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("accept");
            continue;
        }
        handleClient(clientSock);
        ::close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    std::string request;
    std::array<char, 4096> buffer{};

    while (true) {
        ssize_t received = recv(clientSock, buffer.data(), buffer.size(), 0);
        if (received <= 0) {
            break;
        }
        request.append(buffer.data(), static_cast<size_t>(received));
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    if (request.empty()) {
        return;
    }

    std::istringstream requestStream(request);
    std::string method;
    std::string path;
    std::string version;
    requestStream >> method >> path >> version;
    path = sanitizePath(path);

    if (method != "GET") {
        sendResponse(clientSock, "HTTP/1.1 405 Method Not Allowed\r\n",
                     "Content-Type: text/plain\r\nConnection: close\r\n\r\n", "Method Not Allowed");
        return;
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/audio") {
        sendAudio(clientSock);
        return;
    }

    if (path.find("..") != std::string::npos) {
        send404(clientSock);
        return;
    }

    std::string resolvedPath = "public";
    if (path == "/") {
        resolvedPath += "/index.html";
    } else {
        resolvedPath += path;
    }

    sendFile(clientSock, resolvedPath, getMimeType(resolvedPath));
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        send404(clientSock);
        return;
    }

    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::ostringstream headerStream;
    headerStream << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: " << mimeType << "\r\n"
                 << "Content-Length: " << size << "\r\n"
                 << "Cache-Control: no-store\r\n"
                 << "Connection: close\r\n\r\n";

    const std::string header = headerStream.str();
    if (!sendAll(clientSock, header.c_str(), header.size())) {
        return;
    }

    std::array<char, kChunkSize> buffer{};
    while (file) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) {
            break;
        }
        if (!sendAll(clientSock, buffer.data(), static_cast<size_t>(bytesRead))) {
            break;
        }
    }
}

void Server::sendAudio(int clientSock) {
    const std::string audioPath = "data/track.wav";
    int fd = ::open(audioPath.c_str(), O_RDONLY);
    if (fd < 0) {
        send500(clientSock, "Unable to open audio file");
        return;
    }

    struct stat st {};
    if (fstat(fd, &st) < 0) {
        ::close(fd);
        send500(clientSock, "Unable to stat audio file");
        return;
    }

    std::ostringstream headerStream;
    headerStream << "HTTP/1.1 200 OK\r\n"
                 << "Content-Type: audio/wav\r\n"
                 << "Content-Length: " << st.st_size << "\r\n"
                 << "Cache-Control: no-store\r\n"
                 << "Connection: close\r\n\r\n";

    const std::string header = headerStream.str();
    if (!sendAll(clientSock, header.c_str(), header.size())) {
        ::close(fd);
        return;
    }

    std::array<char, kChunkSize> buffer{};
    while (true) {
        ssize_t bytesRead = ::read(fd, buffer.data(), buffer.size());
        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (bytesRead == 0) {
            break;
        }
        if (!sendAll(clientSock, buffer.data(), static_cast<size_t>(bytesRead))) {
            break;
        }
    }

    ::close(fd);
}

void Server::sendResponse(int clientSock, const std::string& statusLine, const std::string& headers,
                          const std::string& body) {
    std::ostringstream response;
    response << statusLine << headers << body;
    const std::string payload = response.str();
    sendAll(clientSock, payload.c_str(), payload.size());
}

void Server::send404(int clientSock) {
    const std::string body = "<h1>404 Not Found</h1>";
    sendResponse(clientSock, "HTTP/1.1 404 Not Found\r\n",
                 "Content-Type: text/html\r\nContent-Length: " + std::to_string(body.size()) +
                     "\r\nConnection: close\r\n\r\n",
                 body);
}

void Server::send500(int clientSock, const std::string& message) {
    const std::string body = "<h1>500 Internal Server Error</h1><p>" + message + "</p>";
    sendResponse(clientSock, "HTTP/1.1 500 Internal Server Error\r\n",
                 "Content-Type: text/html\r\nContent-Length: " + std::to_string(body.size()) +
                     "\r\nConnection: close\r\n\r\n",
                 body);
}

std::string Server::getMimeType(const std::string& path) const {
    auto endsWith = [](const std::string& str, const std::string& suffix) {
        if (suffix.size() > str.size()) {
            return false;
        }
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    if (endsWith(path, ".html")) {
        return "text/html";
    }
    if (endsWith(path, ".css")) {
        return "text/css";
    }
    if (endsWith(path, ".js")) {
        return "application/javascript";
    }
    if (endsWith(path, ".json")) {
        return "application/json";
    }
    if (endsWith(path, ".png")) {
        return "image/png";
    }
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) {
        return "image/jpeg";
    }
    if (endsWith(path, ".svg")) {
        return "image/svg+xml";
    }
    return "application/octet-stream";
}

bool Server::sendAll(int sock, const char* data, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
        ssize_t sent = ::send(sock, data + totalSent, length - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}
