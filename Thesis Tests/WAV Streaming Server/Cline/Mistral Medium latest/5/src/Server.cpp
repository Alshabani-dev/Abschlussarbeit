#include "Server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

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

    std::cout << "Server listening on port " << port_ << std::endl;
}

void Server::run() {
    running_ = true;
    setupSocket();

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSock < 0) {
            if (errno == EINTR) continue;
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
        }

        std::cout << "Client connected from " << inet_ntoa(clientAddr.sin_addr) << std::endl;

        try {
            handleClient(clientSock);
        } catch (const std::exception& e) {
            std::cerr << "Error handling client: " << e.what() << std::endl;
        }

        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    std::array<char, 4096> buffer{};
    ssize_t bytesRead = recv(clientSock, buffer.data(), buffer.size() - 1, 0);

    if (bytesRead <= 0) {
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer.data());

    size_t methodEnd = request.find(' ');
    if (methodEnd == std::string::npos) {
        send404(clientSock);
        return;
    }

    size_t pathEnd = request.find(' ', methodEnd + 1);
    if (pathEnd == std::string::npos) {
        send404(clientSock);
        return;
    }

    std::string path = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);

    if (path == "/") {
        path = "/index.html";
    }

    handleGet(path, clientSock);
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/audio") {
        sendAudio(clientSock);
    } else {
        std::string filePath = "public" + path;
        std::string mimeType = getMimeType(filePath);

        std::ifstream file(filePath, std::ios::binary);
        if (file) {
            sendFile(clientSock, filePath, mimeType);
        } else {
            send404(clientSock);
        }
    }
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        send404(clientSock);
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: " << mimeType << "\r\n";
    header << "Content-Length: " << size << "\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";

    std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }

    std::array<char, 65536> buffer{};
    while (file) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (!sendAll(clientSock, buffer.data(), bytesRead)) {
                return;
            }
        }
    }
}

void Server::sendAudio(int clientSock) {
    std::string filePath = "data/track.wav";
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        send404(clientSock);
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: audio/wav\r\n";
    header << "Content-Length: " << size << "\r\n";
    header << "Cache-Control: no-store\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";

    std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }

    std::array<char, 65536> buffer{};
    while (file) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (!sendAll(clientSock, buffer.data(), bytesRead)) {
                return;
            }
        }
    }
}

void Server::send404(int clientSock) {
    std::string response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>404 Not Found</h1></body></html>";

    sendAll(clientSock, response.c_str(), response.size());
}

void Server::send500(int clientSock, const std::string& message) {
    std::string response =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>500 Internal Server Error</h1><p>" + message + "</p></body></html>";

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
