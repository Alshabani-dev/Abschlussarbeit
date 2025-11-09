#include "Server.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <array>

namespace {
constexpr size_t kBufferSize = 65536;
const std::string kAudioPath = "data/track.wav";
}

Server::Server(int port)
    : port_(port), serverSock_(-1), running_(false) {
    std::signal(SIGPIPE, SIG_IGN);
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

    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Bind failed");
    }

    if (listen(serverSock_, 10) < 0) {
        throw std::runtime_error("Listen failed");
    }
}

void Server::run() {
    running_ = true;
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientSock = accept(serverSock_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
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
    std::string request;
    std::array<char, 4096> buffer{};
    while (true) {
        ssize_t received = recv(clientSock, buffer.data(), buffer.size(), 0);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("recv");
            return;
        }
        if (received == 0) {
            break;
        }
        request.append(buffer.data(), static_cast<size_t>(received));
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    std::istringstream stream(request);
    std::string method;
    std::string path;
    stream >> method >> path;
    if (method != "GET" || path.empty()) {
        send404(clientSock);
        return;
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/" || path == "/index.html") {
        sendFile(clientSock, "public/index.html", getMimeType(".html"));
    } else if (path == "/styles.css") {
        sendFile(clientSock, "public/styles.css", getMimeType(".css"));
    } else if (path == "/app.js") {
        sendFile(clientSock, "public/app.js", getMimeType(".js"));
    } else if (path == "/audio") {
        sendAudio(clientSock);
    } else {
        send404(clientSock);
    }
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
    return "application/octet-stream";
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

    std::ostringstream headers;
    headers << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: " << mimeType << "\r\n"
            << "Content-Length: " << size << "\r\n"
            << "Cache-Control: no-store\r\n"
            << "Connection: close\r\n\r\n";

    const std::string headerStr = headers.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }

    std::array<char, kBufferSize> buffer{};
    while (file) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytes = file.gcount();
        if (bytes <= 0) {
            break;
        }
        if (!sendAll(clientSock, buffer.data(), static_cast<size_t>(bytes))) {
            break;
        }
    }
}

void Server::sendAudio(int clientSock) {
    int fd = open(kAudioPath.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    struct stat st {};
    if (fstat(fd, &st) != 0) {
        close(fd);
        send500(clientSock, "Failed to stat audio file");
        return;
    }

    std::ostringstream headers;
    headers << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: audio/wav\r\n"
            << "Content-Length: " << st.st_size << "\r\n"
            << "Cache-Control: no-store\r\n"
            << "Connection: close\r\n\r\n";

    const std::string headerStr = headers.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        close(fd);
        return;
    }

    std::array<char, kBufferSize> buffer{};
    while (true) {
        ssize_t readBytes = read(fd, buffer.data(), buffer.size());
        if (readBytes < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("read");
            break;
        }
        if (readBytes == 0) {
            break;
        }
        if (!sendAll(clientSock, buffer.data(), static_cast<size_t>(readBytes))) {
            break;
        }
    }

    close(fd);
}

void Server::send404(int clientSock) {
    std::string body = "<h1>404 Not Found</h1>";
    std::ostringstream headers;
    headers << "HTTP/1.1 404 Not Found\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n\r\n";

    const std::string headerStr = headers.str();
    sendAll(clientSock, headerStr.c_str(), headerStr.size());
    sendAll(clientSock, body.c_str(), body.size());
}

void Server::send500(int clientSock, const std::string& message) {
    std::string body = "<h1>500 Internal Server Error</h1><p>" + message + "</p>";
    std::ostringstream headers;
    headers << "HTTP/1.1 500 Internal Server Error\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n\r\n";

    const std::string headerStr = headers.str();
    sendAll(clientSock, headerStr.c_str(), headerStr.size());
    sendAll(clientSock, body.c_str(), body.size());
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
