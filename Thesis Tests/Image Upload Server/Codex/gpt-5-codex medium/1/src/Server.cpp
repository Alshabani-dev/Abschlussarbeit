#include "Server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "FileHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UploadHandler.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace {
constexpr int kBacklog = 128;
constexpr int kPollTimeoutMs = 1000;

int setNonBlocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

bool shouldCloseOnReadError(int err) {
    return !(err == EAGAIN || err == EWOULDBLOCK || err == EINTR);
}

std::string methodFromBuffer(const std::vector<char>& buffer) {
    const auto it = std::find(buffer.begin(), buffer.end(), ' ');
    if (it == buffer.end()) {
        return {};
    }
    return std::string(buffer.begin(), it);
}

std::ptrdiff_t expectedSizeFromHeaders(const std::string& headers, const std::string& method) {
    const bool isBodyless = method == "GET" || method == "HEAD";

    size_t headerEnd = headers.find("\r\n\r\n");
    size_t delimiterLength = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = headers.find("\n\n");
        delimiterLength = 2;
    }
    if (headerEnd == std::string::npos) {
        return -1;
    }

    const std::ptrdiff_t headerSize = static_cast<std::ptrdiff_t>(headerEnd + delimiterLength);

    if (isBodyless) {
        return headerSize;
    }

    std::string lowered = headers.substr(0, headerEnd);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    const std::string target = "content-length:";
    const auto contentLengthPos = lowered.find(target);
    if (contentLengthPos == std::string::npos) {
        return headerSize;
    }

    size_t valueStart = contentLengthPos + target.size();
    while (valueStart < lowered.size() && std::isspace(static_cast<unsigned char>(lowered[valueStart]))) {
        ++valueStart;
    }
    const size_t valueEnd = lowered.find_first_of("\r\n", valueStart);
    std::string value = headers.substr(valueStart, valueEnd == std::string::npos ? std::string::npos : valueEnd - valueStart);
    value.erase(0, value.find_first_not_of(" \t"));
    const long long contentLength = std::strtoll(value.c_str(), nullptr, 10);
    if (contentLength < 0) {
        return -1;
    }
    return headerSize + static_cast<std::ptrdiff_t>(contentLength);
}

void ensureDefaultHeaders(HttpResponse& response) {
    if (response.statusCode() >= 400) {
        if (response.body().empty()) {
            const std::string body = std::to_string(response.statusCode()) + " " + response.statusMessage();
            response.setBody(body);
        }
    }
    response.setHeader("Connection", "close");
}
} // namespace

Server::Server(int port)
    : port_(port),
      fileHandler_(std::make_unique<FileHandler>("public")),
      uploadHandler_(std::make_unique<UploadHandler>("Data")) {}

Server::~Server() {
    stop();
}

void Server::run() {
    if (running_) {
        return;
    }
    running_ = true;
    ::signal(SIGPIPE, SIG_IGN);
    setupSocket();
    eventLoop();
}

void Server::stop() {
    running_ = false;
    if (serverSock_ != -1) {
        close(serverSock_);
        serverSock_ = -1;
    }
    for (auto& [fd, state] : clients_) {
        (void)state;
        close(fd);
    }
    clients_.clear();
}

void Server::setupSocket() {
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    if (setNonBlocking(serverSock_) == -1) {
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSock_, kBacklog) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::eventLoop() {
    std::vector<pollfd> pollFds;
    pollFds.push_back({serverSock_, POLLIN, 0});

    while (running_) {
        const int ready = ::poll(pollFds.data(), static_cast<nfds_t>(pollFds.size()), kPollTimeoutMs);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Poll failed: " << std::strerror(errno) << '\n';
            break;
        }

        if (pollFds[0].revents & POLLIN) {
            while (true) {
                sockaddr_in clientAddr {};
                socklen_t clientLen = sizeof(clientAddr);
                const int clientSock = accept(serverSock_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
                if (clientSock < 0) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        break;
                    }
                    std::cerr << "Accept failed: " << std::strerror(errno) << '\n';
                    break;
                }
                if (setNonBlocking(clientSock) == -1) {
                    std::cerr << "Failed to mark client socket non-blocking\n";
                    close(clientSock);
                    continue;
                }
                clients_.emplace(clientSock, ClientState{});
                pollFds.push_back({clientSock, POLLIN, 0});
            }
        }

        for (size_t i = 1; i < pollFds.size(); ++i) {
            auto& entry = pollFds[i];
            if (entry.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                closeClient(entry.fd);
                close(entry.fd);
                pollFds.erase(pollFds.begin() + static_cast<std::ptrdiff_t>(i));
                --i;
                continue;
            }

            if (entry.revents & POLLIN) {
                handleClientData(entry.fd);
                if (!clients_.count(entry.fd)) {
                    close(entry.fd);
                    pollFds.erase(pollFds.begin() + static_cast<std::ptrdiff_t>(i));
                    --i;
                }
            }
        }
    }
}

void Server::handleClientData(int clientSock) {
    auto it = clients_.find(clientSock);
    if (it == clients_.end()) {
        return;
    }

    ClientState& state = it->second;
    char buffer[4096];
    while (true) {
        const ssize_t bytes = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            state.buffer.insert(state.buffer.end(), buffer, buffer + bytes);
        } else if (bytes == 0) {
            closeClient(clientSock);
            return;
        } else {
            if (shouldCloseOnReadError(errno)) {
                closeClient(clientSock);
            }
            break;
        }
    }

    if (isRequestComplete(state)) {
        processRequest(clientSock, state);
        closeClient(clientSock);
    }
}

bool Server::isRequestComplete(ClientState& state) {
    const std::string raw(state.buffer.begin(), state.buffer.end());
    size_t headerEnd = raw.find("\r\n\r\n");
    size_t delimiterLength = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = raw.find("\n\n");
        delimiterLength = 2;
    }
    if (headerEnd == std::string::npos) {
        return false;
    }

    if (!state.headersParsed) {
        const std::string method = methodFromBuffer(state.buffer);
        const std::string headers = raw.substr(0, headerEnd + delimiterLength);
        state.expectedSize = expectedSizeFromHeaders(headers, method);
        state.headersParsed = true;
    }

    if (state.expectedSize < 0) {
        return false;
    }

    return static_cast<std::ptrdiff_t>(state.buffer.size()) >= state.expectedSize;
}

void Server::processRequest(int clientSock, ClientState& state) {
    try {
        HttpRequest request = HttpRequest::parse(state.buffer);
        HttpResponse response = routeRequest(request);
        ensureDefaultHeaders(response);
        sendResponse(clientSock, response);
    } catch (const std::exception& ex) {
        HttpResponse response;
        response.setStatus(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody(std::string("400 Bad Request: ") + ex.what());
        ensureDefaultHeaders(response);
        sendResponse(clientSock, response);
    }
}

HttpResponse Server::routeRequest(const HttpRequest& request) {
    const std::string methodUpper = request.method();
    const std::string path = request.path();

    if (methodUpper == "GET" || methodUpper == "HEAD") {
        HttpResponse response = fileHandler_->serve(path);
        if (methodUpper == "HEAD") {
            const auto body = response.body();
            response.setBody(std::vector<char>{});
            response.setHeader("Content-Length", std::to_string(body.size()));
        }
        return response;
    }

    if (methodUpper == "POST" && path == "/") {
        const std::string contentType = request.headerValue("Content-Type");
        if (contentType.empty()) {
            HttpResponse response;
            response.setStatus(400, "Bad Request");
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody("400 Bad Request: Missing Content-Type");
            return response;
        }
        try {
            return uploadHandler_->handle(request.body(), contentType);
        } catch (const std::exception& ex) {
            HttpResponse response;
            response.setStatus(500, "Internal Server Error");
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody(std::string("500 Internal Server Error: ") + ex.what());
            return response;
        }
    }

    HttpResponse response;
    response.setStatus(405, "Method Not Allowed");
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody("405 Method Not Allowed");
    return response;
}

void Server::sendResponse(int clientSock, const HttpResponse& response) {
    const std::vector<char> data = response.serialize();
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        const ssize_t sent = send(clientSock, data.data() + totalSent, data.size() - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        totalSent += static_cast<size_t>(sent);
    }
}

void Server::closeClient(int clientSock) {
    auto it = clients_.find(clientSock);
    if (it != clients_.end()) {
        clients_.erase(it);
    }
}
