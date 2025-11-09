#include "Server.h"
#include <iostream>
#include <sstream>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cctype>

// C++17 compatible endsWith implementation
bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

Server::Server(int port) : port_(port), serverSock_(-1), running_(true) {
    // Ignore SIGPIPE to prevent crashes when clients disconnect
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

    // Allow quick restart of server
    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSock_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSock_, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Server listening on port " << port_ << std::endl;
}

void Server::run() {
    try {
        setupSocket();

        while (running_) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            int clientSock = accept(serverSock_, (struct sockaddr*)&clientAddr, &clientLen);

            if (clientSock < 0) {
                if (errno == EINTR) continue; // Interrupted by signal
                throw std::runtime_error("Failed to accept client connection");
            }

            // Handle client in a separate thread would be better, but for simplicity we'll handle sequentially
            handleClient(clientSock);
            close(clientSock);
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

void Server::handleClient(int clientSock) {
    const int bufferSize = 4096;
    char buffer[bufferSize] = {0};
    ssize_t bytesRead = recv(clientSock, buffer, bufferSize - 1, 0);

    if (bytesRead < 0) {
        send500(clientSock, "Failed to read request");
        return;
    }

    // Parse the HTTP request
    std::string request(buffer, bytesRead);
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

    // Handle GET requests
    if (request.substr(0, methodEnd) == "GET") {
        handleGet(path, clientSock);
    } else {
        send404(clientSock);
    }
}

void Server::handleGet(const std::string& path, int clientSock) {
    if (path == "/" || path.empty()) {
        sendFile(clientSock, "public/index.html", "text/html");
    } else if (path == "/audio") {
        sendAudio(clientSock);
    } else if (path == "/app.js") {
        sendFile(clientSock, "public/app.js", "application/javascript");
    } else if (path == "/styles.css") {
        sendFile(clientSock, "public/styles.css", "text/css");
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

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Prepare HTTP response header
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: " << mimeType << "\r\n";
    header << "Content-Length: " << fileSize << "\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";

    std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        return;
    }

    // Send file content
    const size_t chunkSize = 65536;
    std::array<char, chunkSize> buffer;

    while (file) {
        file.read(buffer.data(), chunkSize);
        size_t bytesRead = file.gcount();
        if (bytesRead > 0) {
            if (!sendAll(clientSock, buffer.data(), bytesRead)) {
                return;
            }
        }
    }
}

void Server::sendAudio(int clientSock) {
    const std::string audioPath = "data/track.wav";
    int fd = open(audioPath.c_str(), O_RDONLY);
    if (fd < 0) {
        send404(clientSock);
        return;
    }

    // Get file stats
    struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
        close(fd);
        send500(clientSock, "Failed to get file stats");
        return;
    }

    // Prepare HTTP response header
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n";
    header << "Content-Type: audio/wav\r\n";
    header << "Content-Length: " << fileStat.st_size << "\r\n";
    header << "Cache-Control: no-store\r\n";
    header << "Connection: close\r\n";
    header << "\r\n";

    std::string headerStr = header.str();
    if (!sendAll(clientSock, headerStr.c_str(), headerStr.size())) {
        close(fd);
        return;
    }

    // Stream file in chunks
    const size_t chunkSize = 65536;
    std::array<char, chunkSize> buffer;
    off_t offset = 0;
    size_t remaining = fileStat.st_size;

    while (remaining > 0) {
        size_t toRead = std::min(chunkSize, remaining);
        ssize_t bytesRead = pread(fd, buffer.data(), toRead, offset);
        if (bytesRead < 0) {
            close(fd);
            return;
        }

        if (!sendAll(clientSock, buffer.data(), bytesRead)) {
            close(fd);
            return;
        }

        offset += bytesRead;
        remaining -= bytesRead;
    }

    close(fd);
}

void Server::send404(int clientSock) {
    const char* response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "404 Not Found";

    sendAll(clientSock, response, strlen(response));
}

void Server::send500(int clientSock, const std::string& message) {
    std::ostringstream response;
    response << "HTTP/1.1 500 Internal Server Error\r\n"
             << "Content-Type: text/plain\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << "500 Internal Server Error: " << message;

    std::string responseStr = response.str();
    sendAll(clientSock, responseStr.c_str(), responseStr.size());
}

std::string Server::getMimeType(const std::string& path) const {
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
