#include "Server.h"
#include <iostream>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sstream>
#include <algorithm>
#include <functional>

Server::Server(int port) : port_(port), serverSock_(-1), running_(false) {
    signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSock_ != -1) {
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
        close(serverSock_);
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSock_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSock_, 10) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::run() {
    running_ = true;
    setupSocket();
    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSock < 0) {
            if (errno == EINTR) continue;
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
        handleClient(clientSock);
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    char buffer[4096] = {0};
    ssize_t bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        return;
    }

    std::string request(buffer, bytesRead);
    std::cout << "Received request: " << request << std::endl;

    size_t getPos = request.find("GET ");
    size_t endPos = request.find(" HTTP/");

    if (getPos == std::string::npos || endPos == std::string::npos) {
        std::cout << "Invalid request format" << std::endl;
        send404(clientSock);
        return;
    }

    std::string path = request.substr(getPos + 4, endPos - (getPos + 4));
    std::cout << "Requested path: " << path << std::endl;

    // Check if path is empty or just "/"
    if (path.empty() || path == "/") {
        path = "/index.html";
        std::cout << "Redirecting to index.html" << std::endl;
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    std::cout << "Handling path: " << path << std::endl;

    // Handle root and index.html
    if (path == "/" || path == "/index.html") {
        std::cout << "Serving index.html" << std::endl;
        sendFile(clientSock, "public/index.html", "text/html");
        return;
    }

    // Handle static files
    if (path == "/app.js") {
        std::cout << "Serving app.js" << std::endl;
        sendFile(clientSock, "public/app.js", "application/javascript");
        return;
    }

    if (path == "/styles.css") {
        std::cout << "Serving styles.css" << std::endl;
        sendFile(clientSock, "public/styles.css", "text/css");
        return;
    }

    // Handle audio streaming
    if (path == "/audio") {
        std::cout << "Streaming audio" << std::endl;
        sendAudio(clientSock);
        return;
    }

    // 404 for everything else
    std::cout << "404 Not Found for path: " << path << std::endl;
    send404(clientSock);
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        send404(clientSock);
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: " << mimeType << "\r\n";
    header << "Content-Length: " << fileSize << "\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";

    if (!sendAll(clientSock, header.str().c_str(), header.str().size())) {
        return;
    }

    std::array<char, 65536> buffer;
    while (file) {
        file.read(buffer.data(), buffer.size());
        size_t bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (!sendAll(clientSock, buffer.data(), bytesRead)) {
                return;
            }
        }
    }
}

void Server::sendAudio(int clientSock) {
    int fd = open("data/track.wav", O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    struct stat statBuf;
    if (fstat(fd, &statBuf) < 0) {
        close(fd);
        send500(clientSock, "Failed to get file stats");
        return;
    }

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: audio/wav\r\n";
    header << "Content-Length: " << statBuf.st_size << "\r\n";
    header << "Cache-Control: no-store\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";

    if (!sendAll(clientSock, header.str().c_str(), header.str().size())) {
        close(fd);
        return;
    }

    off_t offset = 0;
    size_t remaining = statBuf.st_size;

    while (remaining > 0) {
        size_t chunkSize = std::min(remaining, static_cast<size_t>(65536));
        ssize_t sent = sendfile(clientSock, fd, &offset, chunkSize);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            break;
        }
        remaining -= sent;
    }

    close(fd);
}

void Server::send404(int clientSock) {
    std::string response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "404 Not Found";

    sendAll(clientSock, response.c_str(), response.size());
}

void Server::send500(int clientSock, const std::string& message) {
    std::string response =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "500 Internal Server Error: " + message;

    sendAll(clientSock, response.c_str(), response.size());
}

std::string Server::getMimeType(const std::string& path) const {
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
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
            return false;
        }
        totalSent += sent;
    }
    return true;
}
