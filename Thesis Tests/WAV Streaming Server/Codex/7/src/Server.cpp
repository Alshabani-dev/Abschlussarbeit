#include "Server.h"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace {
constexpr int kBacklog = 16;
constexpr const char* kAudioPath = "data/track.wav";
}

Server::Server(int port) : port_(port), serverSock_(-1), running_(false) {
    signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSock_ >= 0) {
        close(serverSock_);
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

void Server::setupSocket() {
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("bind() failed");
    }

    if (listen(serverSock_, kBacklog) < 0) {
        throw std::runtime_error("listen() failed");
    }
}

void Server::handleClient(int clientSock) {
    std::array<char, 4096> buffer{};
    ssize_t received = recv(clientSock, buffer.data(), buffer.size() - 1, 0);
    if (received <= 0) {
        return;
    }

    std::string request(buffer.data(), static_cast<size_t>(received));
    std::istringstream requestStream(request);
    std::string method;
    std::string path;

    requestStream >> method >> path;
    if (method.empty() || path.empty()) {
        send500(clientSock, "Malformed request");
        return;
    }

    if (method != "GET") {
        send405(clientSock);
        return;
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/audio") {
        sendAudio(clientSock);
        return;
    }

    std::string relative = sanitizePath(path);
    if (relative.empty()) {
        send404(clientSock);
        return;
    }

    std::string filePath = "public/" + relative;
    sendFile(clientSock, filePath, getMimeType(relative));
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

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: " << mimeType << "\r\n"
           << "Content-Length: " << size << "\r\n"
           << "Connection: close\r\n"
           << "\r\n";
    std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }

    std::array<char, 8192> buffer{};
    while (file) {
        file.read(buffer.data(), buffer.size());
        std::streamsize count = file.gcount();
        if (count <= 0) {
            break;
        }

        if (!sendAll(clientSock, buffer.data(), static_cast<size_t>(count))) {
            break;
        }
    }
}

void Server::sendAudio(int clientSock) {
    int fd = open(kAudioPath, O_RDONLY);
    if (fd < 0) {
        send500(clientSock, "Audio file not found");
        return;
    }

    struct stat st{};
    if (fstat(fd, &st) < 0) {
        close(fd);
        send500(clientSock, "Unable to stat audio file");
        return;
    }

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: audio/wav\r\n"
           << "Content-Length: " << st.st_size << "\r\n"
           << "Cache-Control: no-store\r\n"
           << "Connection: close\r\n"
           << "\r\n";
    std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        close(fd);
        return;
    }

    std::array<char, 65536> buffer{};
    ssize_t bytesRead = 0;
    while ((bytesRead = read(fd, buffer.data(), buffer.size())) > 0) {
        if (!sendAll(clientSock, buffer.data(), static_cast<size_t>(bytesRead))) {
            break;
        }
    }

    close(fd);
}

void Server::send404(int clientSock) {
    const char body[] = "<h1>404 Not Found</h1>";
    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << sizeof(body) - 1 << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;
    std::string respStr = response.str();
    sendAll(clientSock, respStr.c_str(), respStr.size());
}

void Server::send500(int clientSock, const std::string& message) {
    std::string body = "<h1>500 Internal Server Error</h1><p>" + message + "</p>";
    std::ostringstream response;
    response << "HTTP/1.1 500 Internal Server Error\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;
    std::string respStr = response.str();
    sendAll(clientSock, respStr.c_str(), respStr.size());
}

void Server::send405(int clientSock) {
    const char body[] = "<h1>405 Method Not Allowed</h1>";
    std::ostringstream response;
    response << "HTTP/1.1 405 Method Not Allowed\r\n"
             << "Allow: GET\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << sizeof(body) - 1 << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;
    std::string respStr = response.str();
    sendAll(clientSock, respStr.c_str(), respStr.size());
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
    return "application/octet-stream";
}

std::string Server::sanitizePath(const std::string& path) const {
    std::string cleanPath = path;
    auto pos = cleanPath.find('?');
    if (pos != std::string::npos) {
        cleanPath = cleanPath.substr(0, pos);
    }

    if (cleanPath == "/" || cleanPath.empty()) {
        return "index.html";
    }

    if (cleanPath.front() == '/') {
        cleanPath.erase(0, 1);
    }

    if (cleanPath.find("..") != std::string::npos) {
        return {};
    }

    return cleanPath;
}
