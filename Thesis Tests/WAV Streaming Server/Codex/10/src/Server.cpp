#include "Server.h"

#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>

namespace {
constexpr const char* kIndexPath = "public/index.html";
constexpr const char* kCssPath = "public/styles.css";
constexpr const char* kJsPath = "public/app.js";
constexpr const char* kAudioPath = "data/track.wav";
}

Server::Server(int port)
    : port_(port), serverSock_(-1), running_(false) {
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
    addr.sin_port = htons(port_);

    if (bind(serverSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSock_, 16) < 0) {
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

    while (request.find("\r\n\r\n") == std::string::npos) {
        ssize_t received = recv(clientSock, buffer.data(), buffer.size(), 0);
        if (received <= 0) {
            return;
        }
        request.append(buffer.data(), static_cast<size_t>(received));
        if (request.size() > 8192) {
            break;
        }
    }

    std::istringstream requestStream(request);
    std::string method;
    std::string path;
    std::string version;
    requestStream >> method >> path >> version;

    if (method != "GET" || path.empty()) {
        send404(clientSock);
        return;
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/" || path == "/index.html") {
        sendFile(clientSock, kIndexPath, getMimeType(kIndexPath));
    } else if (path == "/styles.css") {
        sendFile(clientSock, kCssPath, getMimeType(kCssPath));
    } else if (path == "/app.js") {
        sendFile(clientSock, kJsPath, getMimeType(kJsPath));
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

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string body(static_cast<size_t>(size), '\0');
    if (size > 0) {
        file.read(body.data(), size);
    }

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: " << mimeType << "\r\n"
           << "Content-Length: " << size << "\r\n"
           << "Cache-Control: no-store\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }

    if (!body.empty()) {
        sendAll(clientSock, body.data(), body.size());
    }
}

void Server::sendAudio(int clientSock) {
    int fd = open(kAudioPath, O_RDONLY);
    if (fd < 0) {
        send500(clientSock, "Failed to open audio file");
        return;
    }

    struct stat st {};
    if (fstat(fd, &st) < 0) {
        close(fd);
        send500(clientSock, "Failed to stat audio file");
        return;
    }

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: audio/wav\r\n"
           << "Content-Length: " << st.st_size << "\r\n"
           << "Cache-Control: no-store\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
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
    const std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
    std::ostringstream header;
    header << "HTTP/1.1 404 Not Found\r\n"
           << "Content-Type: text/html; charset=UTF-8\r\n"
           << "Content-Length: " << body.size() << "\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }
    sendAll(clientSock, body.c_str(), body.size());
}

void Server::send500(int clientSock, const std::string& message) {
    std::ostringstream body;
    body << "<html><body><h1>500 Internal Server Error</h1><p>" << message << "</p></body></html>";
    const std::string bodyStr = body.str();

    std::ostringstream header;
    header << "HTTP/1.1 500 Internal Server Error\r\n"
           << "Content-Type: text/html; charset=UTF-8\r\n"
           << "Content-Length: " << bodyStr.size() << "\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }
    sendAll(clientSock, bodyStr.c_str(), bodyStr.size());
}

std::string Server::getMimeType(const std::string& path) const {
    auto endsWith = [](const std::string& str, const std::string& suffix) {
        if (suffix.size() > str.size()) {
            return false;
        }
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    };

    if (endsWith(path, ".html")) {
        return "text/html; charset=UTF-8";
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
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}
