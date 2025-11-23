#include "Server.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <array>
#include <sstream>

Server::Server(int port) : port_(port), serverSock_(-1), running_(false) {
    // Ignore SIGPIPE to prevent crashes when client disconnects
    signal(SIGPIPE, SIG_IGN);
    setupSocket();
}

Server::~Server() {
    running_ = false;
    if (serverSock_ >= 0) {
        close(serverSock_);
    }
}

void Server::setupSocket() {
    // Create socket
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set SO_REUSEADDR for quick restart
    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    // Bind to port
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverSock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to bind to port " + std::to_string(port_));
    }

    // Listen for connections
    if (listen(serverSock_, 10) < 0) {
        close(serverSock_);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "🚀 Server listening on http://localhost:" << port_ << std::endl;
}

void Server::run() {
    running_ = true;
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock_, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSock < 0) {
            if (errno == EINTR) continue;
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            continue;
        }

        handleClient(clientSock);
        close(clientSock);
    }
}

void Server::handleClient(int clientSock) {
    // Read request
    std::array<char, 8192> buffer;
    ssize_t bytesRead = recv(clientSock, buffer.data(), buffer.size() - 1, 0);
    
    if (bytesRead <= 0) {
        return;
    }

    buffer[bytesRead] = '\0';
    std::string request(buffer.data());

    // Parse request line
    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;

    if (method == "GET") {
        handleGet(path, clientSock);
    } else {
        send404(clientSock);
    }
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/") {
        sendFile(clientSock, "public/index.html", "text/html");
    } else if (path == "/audio") {
        sendAudio(clientSock);
    } else if (path == "/styles.css") {
        sendFile(clientSock, "public/styles.css", "text/css");
    } else if (path == "/app.js") {
        sendFile(clientSock, "public/app.js", "application/javascript");
    } else {
        send404(clientSock);
    }
}

void Server::sendFile(int clientSock, const std::string& filePath, const std::string& mimeType) {
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    // Get file size
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        send500(clientSock, "Failed to get file stats");
        return;
    }

    // Send headers
    std::ostringstream headers;
    headers << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: " << mimeType << "\r\n"
            << "Content-Length: " << fileStat.st_size << "\r\n"
            << "Connection: close\r\n"
            << "\r\n";
    
    std::string headerStr = headers.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.length())) {
        close(fd);
        return;
    }

    // Send file content
    std::array<char, 65536> buffer;
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer.data(), buffer.size())) > 0) {
        if (!sendAll(clientSock, buffer.data(), bytesRead)) {
            break;
        }
    }

    close(fd);
}

void Server::sendAudio(int clientSock) {
    const std::string audioPath = "data/track.wav";
    int fd = open(audioPath.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    // Get file size using fstat
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        send500(clientSock, "Failed to get audio file stats");
        return;
    }

    // Send HTTP headers
    std::ostringstream headers;
    headers << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: audio/wav\r\n"
            << "Content-Length: " << fileStat.st_size << "\r\n"
            << "Cache-Control: no-store\r\n"
            << "Connection: close\r\n"
            << "\r\n";
    
    std::string headerStr = headers.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.length())) {
        close(fd);
        return;
    }

    // Stream file in 64KB chunks (binary-safe)
    std::array<char, 65536> buffer;
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer.data(), buffer.size())) > 0) {
        if (!sendAll(clientSock, buffer.data(), bytesRead)) {
            break;
        }
    }

    close(fd);
}

void Server::send404(int clientSock) {
    const char* response = 
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>404 Not Found</h1></body></html>";
    sendAll(clientSock, response, std::strlen(response));
}

void Server::send500(int clientSock, const std::string& message) {
    std::ostringstream response;
    response << "HTTP/1.1 500 Internal Server Error\r\n"
             << "Content-Type: text/html\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << "<html><body><h1>500 Internal Server Error</h1><p>" 
             << message << "</p></body></html>";
    
    std::string responseStr = response.str();
    sendAll(clientSock, responseStr.c_str(), responseStr.length());
}

std::string Server::getMimeType(const std::string& path) const {
    // C++17-compatible endsWith lambda
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
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return false;
        }
        totalSent += sent;
    }
    return true;
}
