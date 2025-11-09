#include "Server.h"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

namespace {
constexpr const char* kPublicDir = "public";
constexpr const char* kAudioPath = "data/track.wav";

bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) {
        return false;
    }
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
}  // namespace

Server::Server(int port)
    : port_(port),
      serverSock_(-1),
      running_(false) {
    std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSock_ >= 0) {
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
    setupSocket();
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
    std::array<char, 4096> buffer{};
    std::string request;
    ssize_t bytesReceived = 0;

    do {
        bytesReceived = recv(clientSock, buffer.data(), buffer.size(), 0);
        if (bytesReceived < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("recv");
            return;
        }
        if (bytesReceived == 0) {
            return;
        }
        request.append(buffer.data(), static_cast<size_t>(bytesReceived));
    } while (request.find("\r\n\r\n") == std::string::npos && request.size() < 65536);

    std::istringstream requestStream(request);
    std::string method;
    std::string path;
    std::string version;
    requestStream >> method >> path >> version;

    if (method != "GET") {
        send404(clientSock);
        return;
    }

    handleGet(path.empty() ? "/" : path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/audio") {
        sendAudio(clientSock);
        return;
    }

    std::string sanitizedPath = path;
    if (sanitizedPath.find("..") != std::string::npos) {
        send404(clientSock);
        return;
    }

    std::string filePath;
    if (sanitizedPath == "/") {
        filePath = std::string(kPublicDir) + "/index.html";
    } else {
        filePath = std::string(kPublicDir) + sanitizedPath;
    }

    std::string mimeType = getMimeType(filePath);
    sendFile(clientSock, filePath, mimeType);
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        send404(clientSock);
        return;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 0) {
        send500(clientSock, "Failed to read file");
        return;
    }

    std::string headers = "HTTP/1.1 200 OK\r\n";
    headers += "Content-Type: " + mimeType + "\r\n";
    headers += "Content-Length: " + std::to_string(size) + "\r\n";
    headers += "Cache-Control: no-store\r\n";
    headers += "Connection: close\r\n\r\n";

    if (!sendAll(clientSock, headers.c_str(), headers.size())) {
        return;
    }

    std::vector<char> buffer(static_cast<size_t>(size));
    file.read(buffer.data(), size);
    if (!file) {
        send500(clientSock, "Failed to read file contents");
        return;
    }

    sendAll(clientSock, buffer.data(), buffer.size());
}

void Server::sendAudio(int clientSock) {
    int audioFd = open(kAudioPath, O_RDONLY);
    if (audioFd < 0) {
        send500(clientSock, "Audio file not found");
        return;
    }

    struct stat st{};
    if (fstat(audioFd, &st) < 0) {
        close(audioFd);
        send500(clientSock, "Failed to stat audio file");
        return;
    }

    std::string headers = "HTTP/1.1 200 OK\r\n";
    headers += "Content-Type: audio/wav\r\n";
    headers += "Content-Length: " + std::to_string(static_cast<long long>(st.st_size)) + "\r\n";
    headers += "Cache-Control: no-store\r\n";
    headers += "Connection: close\r\n\r\n";

    if (!sendAll(clientSock, headers.c_str(), headers.size())) {
        close(audioFd);
        return;
    }

    std::array<char, 65536> buffer{};
    ssize_t bytesRead = 0;
    while ((bytesRead = read(audioFd, buffer.data(), buffer.size())) > 0) {
        if (!sendAll(clientSock, buffer.data(), static_cast<size_t>(bytesRead))) {
            break;
        }
    }

    if (bytesRead < 0) {
        std::perror("read");
    }

    close(audioFd);
}

void Server::send404(int clientSock) {
    const std::string body = "<h1>404 Not Found</h1>";
    std::string headers = "HTTP/1.1 404 Not Found\r\n";
    headers += "Content-Type: text/html\r\n";
    headers += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    headers += "Connection: close\r\n\r\n";

    sendAll(clientSock, headers.c_str(), headers.size());
    sendAll(clientSock, body.c_str(), body.size());
}

void Server::send500(int clientSock, const std::string& message) {
    const std::string body = "<h1>500 Internal Server Error</h1><p>" + message + "</p>";
    std::string headers = "HTTP/1.1 500 Internal Server Error\r\n";
    headers += "Content-Type: text/html\r\n";
    headers += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    headers += "Connection: close\r\n\r\n";

    sendAll(clientSock, headers.c_str(), headers.size());
    sendAll(clientSock, body.c_str(), body.size());
}

std::string Server::getMimeType(const std::string& path) const {
    if (endsWith(path, ".html")) {
        return "text/html";
    }
    if (endsWith(path, ".css")) {
        return "text/css";
    }
    if (endsWith(path, ".js")) {
        return "application/javascript";
    }
    if (endsWith(path, ".png")) {
        return "image/png";
    }
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) {
        return "image/jpeg";
    }
    return "application/octet-stream";
}

bool Server::sendAll(int sock, const char* data, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
        ssize_t sent = send(sock, data + totalSent, length - totalSent, MSG_NOSIGNAL);
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
