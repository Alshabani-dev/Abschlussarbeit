#include "Server.h"

#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr size_t kRequestBufferSize = 4096;
constexpr size_t kStreamChunkSize = 65536;
}

Server::Server(int port)
    : port_(port), serverSock_(-1), running_(false), publicDir_("public"), audioFile_("data/track.wav") {
    signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSock_ >= 0) {
        close(serverSock_);
    }
}

void Server::setupSocket() {
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error(std::string("Failed to create socket: ") + std::strerror(errno));
    }

    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error(std::string("Failed to set socket options: ") + std::strerror(errno));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    if (bind(serverSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("Failed to bind socket: ") + std::strerror(errno));
    }

    if (listen(serverSock_, 16) < 0) {
        throw std::runtime_error(std::string("Failed to listen on socket: ") + std::strerror(errno));
    }
}

void Server::run() {
    setupSocket();
    running_ = true;

    std::cout << "Server listening on http://localhost:" << port_ << std::endl;

    while (running_) {
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int clientSock = accept(serverSock_, reinterpret_cast<sockaddr*>(&client), &len);
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
        send400(clientSock);
        return;
    }

    if (auto pos = path.find('?'); pos != std::string::npos) {
        path = path.substr(0, pos);
    }

    if (method != "GET") {
        send405(clientSock);
        return;
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/" || path == "/index.html") {
        sendFile(clientSock, publicDir_ + "/index.html", "text/html");
        return;
    }

    if (path == "/audio") {
        sendAudio(clientSock);
        return;
    }

    if (path.find("..") != std::string::npos) {
        send404(clientSock);
        return;
    }

    std::string filePath = publicDir_ + path;
    sendFile(clientSock, filePath, getMimeType(filePath));
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        send404(clientSock);
        return;
    }

    std::ostringstream body;
    body << file.rdbuf();
    const std::string bodyStr = body.str();

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: " << mimeType << "\r\n"
           << "Content-Length: " << bodyStr.size() << "\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }
    sendAll(clientSock, bodyStr.c_str(), bodyStr.size());
}

void Server::sendAudio(int clientSock) {
    int fd = open(audioFile_.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    struct stat st {};
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
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
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
        send500(clientSock, "Error reading audio file");
    }

    close(fd);
}

void Server::send404(int clientSock) {
    static const char body[] = "<h1>404 Not Found</h1>";
    std::ostringstream header;
    header << "HTTP/1.1 404 Not Found\r\n"
           << "Content-Type: text/html\r\n"
           << "Content-Length: " << sizeof(body) - 1 << "\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    sendAll(clientSock, headerStr.c_str(), headerStr.size());
    sendAll(clientSock, body, sizeof(body) - 1);
}

void Server::send405(int clientSock) {
    static const char body[] = "<h1>405 Method Not Allowed</h1>";
    std::ostringstream header;
    header << "HTTP/1.1 405 Method Not Allowed\r\n"
           << "Content-Type: text/html\r\n"
           << "Allow: GET\r\n"
           << "Content-Length: " << sizeof(body) - 1 << "\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    sendAll(clientSock, headerStr.c_str(), headerStr.size());
    sendAll(clientSock, body, sizeof(body) - 1);
}

void Server::send500(int clientSock, const std::string& message) {
    std::ostringstream body;
    body << "<h1>500 Internal Server Error</h1><p>" << message << "</p>";
    const std::string bodyStr = body.str();

    std::ostringstream header;
    header << "HTTP/1.1 500 Internal Server Error\r\n"
           << "Content-Type: text/html\r\n"
           << "Content-Length: " << bodyStr.size() << "\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    sendAll(clientSock, headerStr.c_str(), headerStr.size());
    sendAll(clientSock, bodyStr.c_str(), bodyStr.size());
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
    if (endsWith(path, ".svg")) return "image/svg+xml";
    if (endsWith(path, ".ico")) return "image/x-icon";
    if (endsWith(path, ".json")) return "application/json";
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

void Server::send400(int clientSock) {
    static const char body[] = "<h1>400 Bad Request</h1>";
    std::ostringstream header;
    header << "HTTP/1.1 400 Bad Request\r\n"
           << "Content-Type: text/html\r\n"
           << "Content-Length: " << sizeof(body) - 1 << "\r\n"
           << "Connection: close\r\n\r\n";

    const std::string headerStr = header.str();
    sendAll(clientSock, headerStr.c_str(), headerStr.size());
    sendAll(clientSock, body, sizeof(body) - 1);
}
