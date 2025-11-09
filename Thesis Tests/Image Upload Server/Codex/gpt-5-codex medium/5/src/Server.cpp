#include "Server.h"

#include <arpa/inet.h>
#include <csignal>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace {
constexpr int kBacklog = 64;
constexpr int kBufferSize = 4096;

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

std::optional<std::pair<size_t, size_t>> findHeaderDelimiters(const std::vector<char>& buffer) {
    return HttpRequest::findHeaderTerminator(buffer);
}

std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}
} // namespace

Server::Server(int port, std::string publicDir, std::string uploadDir)
    : port_(port),
      publicDir_(std::move(publicDir)),
      uploadDir_(std::move(uploadDir)),
      fileHandler_(publicDir_),
      uploadHandler_(uploadDir_) {
    std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    if (serverSock_ != -1) {
        close(serverSock_);
    }
    for (const auto& [fd, _] : clients_) {
        close(fd);
    }
}

void Server::setupServerSocket() {
    serverSock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ == -1) {
        throw std::runtime_error(std::string("Failed to create socket: ") + std::strerror(errno));
    }

    int enable = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        throw std::runtime_error(std::string("Failed to set socket options: ") + std::strerror(errno));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSock_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
        throw std::runtime_error(std::string("Failed to bind socket: ") + std::strerror(errno));
    }

    if (listen(serverSock_, kBacklog) == -1) {
        throw std::runtime_error(std::string("Failed to listen on socket: ") + std::strerror(errno));
    }

    if (setNonBlocking(serverSock_) == -1) {
        throw std::runtime_error(std::string("Failed to set non-blocking mode: ") + std::strerror(errno));
    }
}

void Server::run() {
    setupServerSocket();
    running_ = true;

    while (running_) {
        std::vector<pollfd> pollFds;
        pollFds.reserve(clients_.size() + 1);
        pollfd serverPoll{};
        serverPoll.fd = serverSock_;
        serverPoll.events = POLLIN;
        pollFds.push_back(serverPoll);

        for (const auto& [fd, _] : clients_) {
            pollfd clientPoll{};
            clientPoll.fd = fd;
            clientPoll.events = POLLIN;
            pollFds.push_back(clientPoll);
        }

        int ready = ::poll(pollFds.data(), pollFds.size(), -1);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("poll() failed");
        }

        if (pollFds[0].revents & POLLIN) {
            acceptNewClients();
        }

        for (size_t i = 1; i < pollFds.size(); ++i) {
            int fd = pollFds[i].fd;
            short revents = pollFds[i].revents;
            if (revents & (POLLERR | POLLHUP | POLLRDHUP)) {
                closeClient(fd);
                continue;
            }

            if (revents & POLLIN) {
                handleClientEvent(fd);
            }
        }
    }
}

void Server::acceptNewClients() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = ::accept(serverSock_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSock == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "Failed to accept client: " << std::strerror(errno) << "\n";
            break;
        }

        if (setNonBlocking(clientSock) == -1) {
            std::cerr << "Failed to set client socket non-blocking\n";
            ::close(clientSock);
            continue;
        }

        clients_.emplace(clientSock, ClientState{});
    }
}

void Server::handleClientEvent(int fd) {
    auto it = clients_.find(fd);
    if (it == clients_.end()) {
        return;
    }

    ClientState& state = it->second;
    char buffer[kBufferSize];

    while (true) {
        ssize_t bytesRead = ::recv(fd, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            state.buffer.insert(state.buffer.end(), buffer, buffer + bytesRead);
        } else if (bytesRead == 0) {
            closeClient(fd);
            return;
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            std::cerr << "recv() failed: " << std::strerror(errno) << "\n";
            closeClient(fd);
            return;
        }
    }

    processBuffer(fd, state);
}

void Server::processBuffer(int fd, ClientState& state) {
    if (!state.headersComplete) {
        auto headerInfo = findHeaderDelimiters(state.buffer);
        if (headerInfo) {
            state.headersComplete = true;
            state.headerLength = headerInfo->first;
            state.headerEnd = headerInfo->first + headerInfo->second;
            std::string headerSection(state.buffer.begin(), state.buffer.begin() + state.headerLength);

            std::string method = toUpper(HttpRequest::extractMethod(headerSection));
            auto contentLength = HttpRequest::extractContentLength(headerSection);

            if (method == "GET" || method == "HEAD") {
                state.expectedBodySize = 0;
            } else {
                if (!contentLength) {
                    HttpResponse response;
                    response.setStatus(411);
                    response.setHeader("Content-Type", "text/plain; charset=utf-8");
                    response.setBody("Length Required");
                    sendResponse(fd, response);
                    closeClient(fd);
                    return;
                }
                state.expectedBodySize = *contentLength;
            }
        }
    }

    if (!state.headersComplete) {
        return;
    }

    if (state.buffer.size() < state.headerEnd) {
        return;
    }

    const size_t bodySize = state.buffer.size() - state.headerEnd;
    if (bodySize < state.expectedBodySize) {
        return;
    }

    const size_t requestSize = state.headerEnd + state.expectedBodySize;
    dispatchRequest(fd, state, requestSize);
}

void Server::dispatchRequest(int fd, ClientState& state, size_t requestSize) {
    std::vector<char> requestData(state.buffer.begin(), state.buffer.begin() + requestSize);

    HttpRequest request;
    if (!request.parse(requestData)) {
        HttpResponse response;
        response.setStatus(400);
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody("Bad Request");
        sendResponse(fd, response);
        closeClient(fd);
        return;
    }

    HttpResponse response;
    const std::string method = toUpper(request.method());

    try {
        if (method == "GET") {
            if (!fileHandler_.serve(request.path(), response)) {
                response.setStatus(404);
                response.setHeader("Content-Type", "text/html; charset=utf-8");
                response.setBody("<h1>404 Not Found</h1>");
            }
        } else if (method == "POST" && request.path() == "/") {
            auto contentType = request.header("content-type");
            if (!contentType) {
                response.setStatus(400);
                response.setHeader("Content-Type", "text/plain; charset=utf-8");
                response.setBody("Missing Content-Type header");
            } else {
                const UploadResult result = uploadHandler_.handle(request.body(), *contentType);
                if (result.success) {
                    response.setStatus(200);
                    response.setHeader("Content-Type", "text/plain; charset=utf-8");
                    response.setBody(result.message);
                } else {
                    response.setStatus(400);
                    response.setHeader("Content-Type", "text/plain; charset=utf-8");
                    response.setBody(result.message);
                }
            }
        } else {
            response.setStatus(405);
            response.setHeader("Allow", "GET, POST");
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody("Method Not Allowed");
        }
    } catch (const std::exception& ex) {
        response.setStatus(500);
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody(std::string("Internal Server Error: ") + ex.what());
    }

    sendResponse(fd, response);
    closeClient(fd);
}

void Server::sendResponse(int fd, const HttpResponse& response) {
    const std::vector<char> bytes = response.serialize();
    size_t totalSent = 0;

    while (totalSent < bytes.size()) {
        ssize_t sent = ::send(fd, bytes.data() + totalSent, bytes.size() - totalSent,
#ifdef MSG_NOSIGNAL
                              MSG_NOSIGNAL
#else
                              0
#endif
        );
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        totalSent += static_cast<size_t>(sent);
    }
}

void Server::closeClient(int fd) {
    auto it = clients_.find(fd);
    if (it != clients_.end()) {
        ::close(fd);
        clients_.erase(it);
    } else {
        ::close(fd);
    }
}
