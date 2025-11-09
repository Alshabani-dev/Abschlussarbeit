#include "Server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr int kBacklog = 10;
constexpr size_t kRequestBuffer = 8192;
}

Server::Server(int port)
    : port_(port), serverSock_(-1), running_(false), publicDir_("public"), dataDir_("data") {
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
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

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
    std::array<char, kRequestBuffer> buffer{};
    ssize_t received = recv(clientSock, buffer.data(), buffer.size() - 1, 0);
    if (received <= 0) {
        return;
    }

    buffer[received] = '\0';
    std::istringstream requestStream(buffer.data());
    std::string method;
    std::string path;
    std::string version;

    requestStream >> method >> path >> version;

    if (method != "GET") {
        sendResponse(clientSock, "HTTP/1.1 405 Method Not Allowed\r\n",
                     "Allow: GET\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", "");
        return;
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/" || path.empty()) {
        sendFile(clientSock, publicDir_ + "/index.html", "text/html");
        return;
    }

    if (path == "/audio") {
        sendAudio(clientSock);
        return;
    }

    std::string filePath = publicDir_ + path;
    std::string mime = getMimeType(path);
    sendFile(clientSock, filePath, mime);
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
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: " << mimeType << "\r\n";
    header << "Content-Length: " << size << "\r\n";
    header << "Cache-Control: no-store\r\n";
    header << "Connection: close\r\n\r\n";
    const std::string headerStr = header.str();

    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }

    std::array<char, 65536> buffer{};
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
    std::string audioPath = dataDir_ + "/track.wav";
    int fd = open(audioPath.c_str(), O_RDONLY);
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
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: audio/wav\r\n";
    header << "Content-Length: " << st.st_size << "\r\n";
    header << "Cache-Control: no-store\r\n";
    header << "Connection: close\r\n\r\n";
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

void Server::sendResponse(int clientSock, const std::string& statusLine, const std::string& headers, const std::string& body) {
    std::string response = statusLine + headers + body;
    sendAll(clientSock, response.c_str(), response.size());
}

void Server::send404(int clientSock) {
    const std::string body = "<h1>404 Not Found</h1>";
    std::ostringstream headers;
    headers << "Content-Type: text/html\r\n";
    headers << "Content-Length: " << body.size() << "\r\n";
    headers << "Connection: close\r\n\r\n";
    sendResponse(clientSock, "HTTP/1.1 404 Not Found\r\n", headers.str(), body);
}

void Server::send500(int clientSock, const std::string& message) {
    const std::string body = "<h1>500 Internal Server Error</h1><p>" + message + "</p>";
    std::ostringstream headers;
    headers << "Content-Type: text/html\r\n";
    headers << "Content-Length: " << body.size() << "\r\n";
    headers << "Connection: close\r\n\r\n";
    sendResponse(clientSock, "HTTP/1.1 500 Internal Server Error\r\n", headers.str(), body);
}

std::string Server::getMimeType(const std::string& path) const {
    auto endsWith = [](const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) {
            return false;
        }
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    };

    if (endsWith(path, ".html")) return "text/html";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    if (endsWith(path, ".png")) return "image/png";
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) return "image/jpeg";
    if (endsWith(path, ".svg")) return "image/svg+xml";
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
            return false;
        }
        totalSent += static_cast<size_t>(sent);
    }
    return true;
}
