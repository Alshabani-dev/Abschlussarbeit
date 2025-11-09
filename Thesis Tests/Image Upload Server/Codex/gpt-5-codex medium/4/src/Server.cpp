#include "Server.h"

#include "FileHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UploadHandler.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
constexpr int kBacklog = 16;
constexpr int kBufferSize = 4096;

bool setNonBlocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return false;
    }
    return true;
}

bool findHeaderBoundary(const std::vector<char>& buffer, size_t& headerLength, size_t& delimiterLength) {
    std::string data(buffer.begin(), buffer.end());
    size_t pos = data.find("\r\n\r\n");
    if (pos != std::string::npos) {
        headerLength = pos;
        delimiterLength = 4;
        return true;
    }
    pos = data.find("\n\n");
    if (pos != std::string::npos) {
        headerLength = pos;
        delimiterLength = 2;
        return true;
    }
    return false;
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

void trim(std::string& value) {
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                value.end());
}
} // namespace

Server::Server(int port)
    : port_(port),
      serverSocket_(-1),
      running_(false),
      fileHandler_(std::make_unique<FileHandler>("public")),
      uploadHandler_(std::make_unique<UploadHandler>()) {}

Server::~Server() {
    if (serverSocket_ >= 0) {
        close(serverSocket_);
    }
}

void Server::configureSignals() {
    std::signal(SIGPIPE, SIG_IGN);
}

void Server::setupListeningSocket() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }
#ifdef SO_REUSEPORT
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEPORT");
    }
#endif

    if (!setNonBlocking(serverSocket_)) {
        throw std::runtime_error("Failed to set listening socket non-blocking");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSocket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("Failed to bind: ") + std::strerror(errno));
    }

    if (listen(serverSocket_, kBacklog) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::run() {
    configureSignals();
    setupListeningSocket();

    std::cout << "Server listening on port " << port_ << "\n";
    running_ = true;

    while (running_) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket_, &readSet);

        int maxFd = serverSocket_;
        for (const auto& [clientFd, _] : clients_) {
            FD_SET(clientFd, &readSet);
            if (clientFd > maxFd) {
                maxFd = clientFd;
            }
        }

        const int ready = select(maxFd + 1, &readSet, nullptr, nullptr, nullptr);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("select failed: ") + std::strerror(errno));
        }

        if (FD_ISSET(serverSocket_, &readSet)) {
            acceptConnections();
        }

        for (auto it = clients_.begin(); it != clients_.end();) {
            const int clientSocket = it->first;
            auto& context = it->second;
            if (FD_ISSET(clientSocket, &readSet)) {
                const bool keep = handleClient(clientSocket, context);
                if (!keep) {
                    closeClient(clientSocket);
                    it = clients_.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }
}

void Server::acceptConnections() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        const int clientSocket = accept(serverSocket_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            std::cerr << "accept failed: " << std::strerror(errno) << "\n";
            break;
        }

        if (!setNonBlocking(clientSocket)) {
            std::cerr << "Failed to set client socket non-blocking\n";
            close(clientSocket);
            continue;
        }

        clients_.emplace(clientSocket, ClientContext{});
    }
}

bool Server::handleClient(int clientSocket, ClientContext& context) {
    char buffer[kBufferSize];
    while (true) {
        const ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            context.buffer.insert(context.buffer.end(), buffer, buffer + bytesRead);
        } else if (bytesRead == 0) {
            return false; // client closed connection
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            std::cerr << "recv failed: " << std::strerror(errno) << "\n";
            return false;
        }
    }

    if (!context.headersParsed) {
        if (findHeaderBoundary(context.buffer, context.headerLength, context.delimiterLength)) {
            context.headersParsed = true;
            std::string headerString(context.buffer.begin(), context.buffer.begin() + context.headerLength);
            std::istringstream headerStream(headerString);
            std::string requestLine;
            if (std::getline(headerStream, requestLine)) {
                if (!requestLine.empty() && requestLine.back() == '\r') {
                    requestLine.pop_back();
                }
                std::istringstream requestLineStream(requestLine);
                std::string method;
                requestLineStream >> method;

                std::string headerLine;
                while (std::getline(headerStream, headerLine)) {
                    if (!headerLine.empty() && headerLine.back() == '\r') {
                        headerLine.pop_back();
                    }
                    if (headerLine.empty()) {
                        continue;
                    }
                    const auto colon = headerLine.find(':');
                    if (colon == std::string::npos) {
                        continue;
                    }
                    std::string key = headerLine.substr(0, colon);
                    std::string value = headerLine.substr(colon + 1);
                    trim(key);
                    trim(value);

                    if (toLower(key) == "content-length") {
                        try {
                            context.expectedContentLength = static_cast<size_t>(std::stoul(value));
                        } catch (const std::exception&) {
                            context.expectedContentLength = 0;
                        }
                    } else if (toLower(key) == "connection") {
                        context.shouldKeepAlive = (toLower(value) == "keep-alive");
                    }
                }

                if (toLower(method) == "get" || toLower(method) == "head") {
                    context.expectedContentLength = 0;
                }
            }
        }
    }

    if (context.headersParsed) {
        const size_t totalExpected =
            context.headerLength + context.delimiterLength + context.expectedContentLength;
        if (context.buffer.size() >= totalExpected) {
            processRequest(clientSocket, context);
            return context.shouldKeepAlive;
        }
    }

    return true;
}

void Server::processRequest(int clientSocket, ClientContext& context) {
    const size_t totalLength =
        context.headerLength + context.delimiterLength + context.expectedContentLength;
    std::vector<char> requestBytes(context.buffer.begin(), context.buffer.begin() + totalLength);

    HttpResponse response;
    try {
        HttpRequest request(requestBytes);

        const std::string method = toLower(request.getMethod());
        if (method == "get") {
            if (!fileHandler_->serve(request.getPath(), response)) {
                response.setStatus(404, "Not Found");
                response.setBodyText(
                    "<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body></html>",
                    "text/html; charset=utf-8");
            }
        } else if (method == "post") {
            if (request.getPath() != "/") {
                response.setStatus(404, "Not Found");
                response.setBodyText(
                    "<html><body><h1>404 Not Found</h1><p>Endpoint not found.</p></body></html>",
                    "text/html; charset=utf-8");
            } else {
                const std::string contentType = request.getHeader("Content-Type");
                const auto result = uploadHandler_->handle(request.getBody(), contentType);
                if (result.success) {
                    response.setStatus(200, "OK");
                    response.setBodyText("<html><body><h1>Upload successful</h1><p>" + result.message +
                                             "</p></body></html>",
                                         "text/html; charset=utf-8");
                } else {
                    response.setStatus(400, "Bad Request");
                    response.setBodyText("<html><body><h1>Upload failed</h1><p>" + result.message +
                                             "</p></body></html>",
                                         "text/html; charset=utf-8");
                }
            }
        } else {
            response.setStatus(405, "Method Not Allowed");
            response.setHeader("allow", "GET, POST");
            response.setBodyText(
                "<html><body><h1>405 Method Not Allowed</h1><p>Supported methods: GET, POST.</p></body></html>",
                "text/html; charset=utf-8");
        }
    } catch (const std::exception& ex) {
        response.setStatus(400, "Bad Request");
        response.setBodyText(std::string("<html><body><h1>400 Bad Request</h1><p>") + ex.what() +
                                 "</p></body></html>",
                             "text/html; charset=utf-8");
    }

    sendResponse(clientSocket, response);
    context.buffer.erase(context.buffer.begin(), context.buffer.begin() + totalLength);
    context.headersParsed = false;
    context.expectedContentLength = 0;
    context.headerLength = 0;
    context.delimiterLength = 0;
    context.shouldKeepAlive = false;
}

void Server::sendResponse(int clientSocket, const HttpResponse& response) {
    const std::vector<char> payload = response.serialize();
    size_t totalSent = 0;
    while (totalSent < payload.size()) {
        ssize_t sent = send(
            clientSocket,
            payload.data() + totalSent,
            payload.size() - totalSent
#ifdef MSG_NOSIGNAL
                ,
            MSG_NOSIGNAL
#else
                ,
            0
#endif
        );
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "send failed: " << std::strerror(errno) << "\n";
            break;
        }
        totalSent += static_cast<size_t>(sent);
    }
}

void Server::closeClient(int clientSocket) {
    close(clientSocket);
}
