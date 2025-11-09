#include "Server.h"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr size_t REQUEST_BUFFER_SIZE = 8192;
constexpr size_t STREAM_CHUNK_SIZE = 64 * 1024; // 64KB
}

Server::Server(int port)
    : port_(port), serverSock_(-1), running_(false) {
    std::signal(SIGPIPE, SIG_IGN);
    auto cwd = std::filesystem::current_path();
    publicDir_ = (cwd / "public").string();
    dataDir_ = (cwd / "data").string();
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
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

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
    std::array<char, REQUEST_BUFFER_SIZE> buffer{};
    ssize_t received = recv(clientSock, buffer.data(), buffer.size() - 1, 0);
    if (received <= 0) {
        return;
    }

    std::string request(buffer.data(), static_cast<size_t>(received));
    auto requestEnd = request.find("\r\n");
    if (requestEnd == std::string::npos) {
        send500(clientSock, "Malformed request");
        return;
    }

    std::string requestLine = request.substr(0, requestEnd);
    std::istringstream iss(requestLine);
    std::string method;
    std::string path;
    iss >> method >> path;

    if (method != "GET") {
        send404(clientSock);
        return;
    }

    if (path.empty()) {
        path = "/";
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

    std::string trimmed = path;
    while (!trimmed.empty() && trimmed.front() == '/') {
        trimmed.erase(trimmed.begin());
    }

    std::filesystem::path requested = std::filesystem::path(publicDir_) / trimmed;
    if (!std::filesystem::exists(requested) || !std::filesystem::is_regular_file(requested)) {
        send404(clientSock);
        return;
    }

    sendFile(clientSock, requested.string(), getMimeType(requested.string()));
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        send500(clientSock, "Unable to open file");
        return;
    }

    std::ostringstream body;
    body << file.rdbuf();
    const std::string bodyStr = body.str();

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << mimeType << "\r\n"
             << "Content-Length: " << bodyStr.size() << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "Connection: close\r\n\r\n";

    const std::string header = response.str();
    if (!sendAll(clientSock, header.c_str(), header.size())) {
        return;
    }

    sendAll(clientSock, bodyStr.data(), bodyStr.size());
}

void Server::sendAudio(int clientSock) {
    const std::string audioPath = dataDir_ + "/track.wav";
    int fd = open(audioPath.c_str(), O_RDONLY);
    if (fd < 0) {
        send500(clientSock, "track.wav missing");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        send500(clientSock, "Unable to stat track.wav");
        return;
    }

    const off_t fileSize = st.st_size;

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: audio/wav\r\n"
             << "Content-Length: " << fileSize << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "Connection: close\r\n\r\n";

    const std::string header = response.str();
    if (!sendAll(clientSock, header.c_str(), header.size())) {
        close(fd);
        return;
    }

    std::array<char, STREAM_CHUNK_SIZE> buffer{};
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
    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;

    const std::string message = response.str();
    sendAll(clientSock, message.c_str(), message.size());
}

void Server::send500(int clientSock, const std::string& message) {
    const std::string body = "<html><body><h1>500 Internal Server Error</h1><p>" + message + "</p></body></html>";
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
    if (endsWith(path, ".svg")) {
        return "image/svg+xml";
    }

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
