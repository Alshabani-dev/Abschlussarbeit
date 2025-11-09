#include "Server.h"

#include "FileHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UploadHandler.h"

#include <arpa/inet.h>
#include <csignal>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace {

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }
    return 0;
}

std::string errnoMessage(const std::string& prefix) {
    return prefix + ": " + std::strerror(errno);
}

HttpResponse makeTextResponse(int statusCode, const std::string& reason, const std::string& message) {
    HttpResponse response(statusCode, reason);
    std::vector<char> body(message.begin(), message.end());
    response.setBody(std::move(body), "text/plain; charset=utf-8");
    response.setHeader("Connection", "close");
    return response;
}

std::string escapeJson(const std::string& input) {
    std::string escaped;
    escaped.reserve(input.size());
    for (char ch : input) {
        if (ch == '"' || ch == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

HttpResponse makeJsonResponse(int statusCode, const std::string& reason, bool success, const std::string& message) {
    HttpResponse response(statusCode, reason);
    std::string payload = std::string("{\"success\":") + (success ? "true" : "false") +
                          ",\"message\":\"" + escapeJson(message) + "\"}";
    std::vector<char> body(payload.begin(), payload.end());
    response.setBody(std::move(body), "application/json; charset=utf-8");
    response.setHeader("Connection", "close");
    return response;
}

}  // namespace

Server::Server(int port)
    : port_(port),
      listenFd_(-1),
      running_(false),
      fileHandler_(std::make_unique<FileHandler>("public")),
      uploadHandler_(std::make_unique<UploadHandler>("Data")) {}

Server::~Server() {
    running_ = false;
    if (listenFd_ >= 0) {
        ::close(listenFd_);
        listenFd_ = -1;
    }
}

void Server::run() {
    if (port_ <= 0 || port_ > 65535) {
        throw std::runtime_error("Port must be between 1 and 65535.");
    }

    std::signal(SIGPIPE, SIG_IGN);

    setupSocket();
    running_ = true;
    std::cout << "Server listening on port " << port_ << std::endl;
    eventLoop();
}

void Server::setupSocket() {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        throw std::runtime_error(errnoMessage("socket failed"));
    }

    int opt = 1;
    if (::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error(errnoMessage("setsockopt SO_REUSEADDR failed"));
    }

    if (setNonBlocking(listenFd_) < 0) {
        throw std::runtime_error(errnoMessage("failed to set listening socket non-blocking"));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(errnoMessage("bind failed"));
    }

    if (::listen(listenFd_, SOMAXCONN) < 0) {
        throw std::runtime_error(errnoMessage("listen failed"));
    }
}

void Server::eventLoop() {
    fd_set masterSet;
    FD_ZERO(&masterSet);
    FD_SET(listenFd_, &masterSet);

    int maxFd = listenFd_;
    std::cout << "Event loop started" << std::endl;

    while (running_) {
        fd_set readSet = masterSet;
        timeval timeout{};
        timeout.tv_sec = 1;

        int ready = ::select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(errnoMessage("select failed"));
        }

        if (ready == 0) {
            continue;
        }

        if (FD_ISSET(listenFd_, &readSet)) {
            acceptNewConnections(masterSet, maxFd);
            if (--ready == 0) {
                continue;
            }
        }

        for (int fd = 0; fd <= maxFd && ready > 0; ++fd) {
            if (fd == listenFd_) {
                continue;
            }
            if (FD_ISSET(fd, &readSet)) {
                --ready;
                handleClientData(fd, masterSet, maxFd);
            }
        }
    }
}

void Server::acceptNewConnections(fd_set& masterSet, int& maxFd) {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = ::accept(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            std::cerr << errnoMessage("accept failed") << std::endl;
            break;
        }

        std::cout << "Accepted client fd " << clientFd << std::endl;
        if (setNonBlocking(clientFd) < 0) {
            std::cerr << errnoMessage("failed to set client socket non-blocking") << std::endl;
            ::close(clientFd);
            continue;
        }

        FD_SET(clientFd, &masterSet);
        if (clientFd > maxFd) {
            maxFd = clientFd;
        }
        clientBuffers_[clientFd] = {};
    }
}

void Server::handleClientData(int clientFd, fd_set& masterSet, int& maxFd) {
    auto bufferIt = clientBuffers_.find(clientFd);
    if (bufferIt == clientBuffers_.end()) {
        clientBuffers_[clientFd] = {};
        bufferIt = clientBuffers_.find(clientFd);
    }
    std::vector<char>& buffer = bufferIt->second;

    bool clientClosed = false;
    char temp[4096];

    while (true) {
        ssize_t received = ::recv(clientFd, temp, sizeof(temp), 0);
        if (received > 0) {
            buffer.insert(buffer.end(), temp, temp + received);
            continue;
        }
        if (received == 0) {
            clientClosed = true;
            break;
        }
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            break;
        }
        std::cerr << errnoMessage("recv failed") << std::endl;
        clientClosed = true;
        break;
    }

    if (clientClosed) {
        closeClient(clientFd, masterSet, maxFd);
        return;
    }

    while (!buffer.empty()) {
        HttpRequest request;
        size_t requestLength = 0;
        try {
            if (!HttpRequest::tryParse(buffer, requestLength, request)) {
                break;
            }
        } catch (const std::exception&) {
            HttpResponse errorResponse(400, "Bad Request");
            static const std::string kMessage = "400 Bad Request";
            std::vector<char> body(kMessage.begin(), kMessage.end());
            errorResponse.setBody(std::move(body), "text/plain; charset=utf-8");
            errorResponse.setHeader("Connection", "close");
            sendResponse(clientFd, errorResponse);
            closeClient(clientFd, masterSet, maxFd);
            buffer.clear();
            std::cerr << "Malformed request, closing client " << clientFd << std::endl;
            return;
        }

        HttpResponse response = routeRequest(request);
        sendResponse(clientFd, response);

        if (requestLength >= buffer.size()) {
            buffer.clear();
        } else {
            buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(requestLength));
        }

        closeClient(clientFd, masterSet, maxFd);
        std::cout << "Completed request from fd " << clientFd << std::endl;
        break;
    }
}

void Server::sendResponse(int clientFd, const HttpResponse& response) {
    std::vector<char> serialized = response.serialize();
    size_t total = serialized.size();
    size_t sent = 0;

    while (sent < total) {
        ssize_t bytes = ::send(clientFd, serialized.data() + sent, total - sent, MSG_NOSIGNAL);
        if (bytes > 0) {
            sent += static_cast<size_t>(bytes);
            continue;
        }
        if (bytes < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
            continue;
        }
        break;
    }
}

HttpResponse Server::routeRequest(const HttpRequest& request) {
    const std::string& method = request.method();

    if (method == "GET" || method == "HEAD") {
        HttpResponse response;
        if (fileHandler_ && fileHandler_->handle(request.path(), response)) {
            response.setHeader("Connection", "close");
            return response;
        }
        return makeTextResponse(404, "Not Found", "404 Not Found");
    }

    if (method == "POST") {
        if (request.path() != "/") {
            return makeJsonResponse(404, "Not Found", false, "Endpoint not found.");
        }

        std::string contentType = request.headerValue("Content-Type");
        if (contentType.empty()) {
            return makeJsonResponse(400, "Bad Request", false, "Missing Content-Type header.");
        }

        if (!uploadHandler_) {
            return makeJsonResponse(500, "Internal Server Error", false, "Upload handler not available.");
        }

        UploadHandler::Result result = uploadHandler_->handle(request.body(), contentType);
        if (!result.success) {
            return makeJsonResponse(400, "Bad Request", false, result.message);
        }

        return makeJsonResponse(200, "OK", true, result.message);
    }

    return makeTextResponse(405, "Method Not Allowed", "Unsupported HTTP method.");
}

void Server::closeClient(int clientFd, fd_set& masterSet, int& maxFd) {
    if (FD_ISSET(clientFd, &masterSet)) {
        FD_CLR(clientFd, &masterSet);
    }
    ::close(clientFd);
    clientBuffers_.erase(clientFd);
    std::cout << "Closed client fd " << clientFd << std::endl;

    if (clientFd == maxFd) {
        while (maxFd > listenFd_ && !FD_ISSET(maxFd, &masterSet)) {
            --maxFd;
        }
    }
}
