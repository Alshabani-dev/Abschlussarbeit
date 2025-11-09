#include "Server.h"

#include "HttpRequest.h"
#include "HttpResponse.h"

#include <arpa/inet.h>
#include <csignal>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {
constexpr int kBacklog = 16;
constexpr int kBufferSize = 8192;

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
} // namespace

Server::Server(int port)
    : port_(port),
      serverSock_(-1),
      fileHandler_("public"),
      uploadHandler_("Data") {}

void Server::run() {
    std::signal(SIGPIPE, SIG_IGN);
    setupSocket();
    eventLoop();
}

void Server::setupSocket() {
    serverSock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error(std::string("Failed to create socket: ") + std::strerror(errno));
    }

    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error(std::string("Failed to set socket options: ") + std::strerror(errno));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("Failed to bind socket: ") + std::strerror(errno));
    }

    if (listen(serverSock_, kBacklog) < 0) {
        throw std::runtime_error(std::string("Failed to listen on socket: ") + std::strerror(errno));
    }

    if (setNonBlocking(serverSock_) < 0) {
        throw std::runtime_error(std::string("Failed to set server socket non-blocking: ") + std::strerror(errno));
    }

    std::cout << "Server listening on port " << port_ << std::endl;
}

void Server::eventLoop() {
    fd_set masterSet;
    FD_ZERO(&masterSet);
    FD_SET(serverSock_, &masterSet);
    int maxFd = serverSock_;

    while (true) {
        fd_set readSet = masterSet;
        int ready = select(maxFd + 1, &readSet, nullptr, nullptr, nullptr);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("select");
            break;
        }

        for (int fd = 0; fd <= maxFd && ready > 0; ++fd) {
            if (!FD_ISSET(fd, &readSet)) {
                continue;
            }
            --ready;

            if (fd == serverSock_) {
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                int clientSock = accept(serverSock_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
                if (clientSock >= 0) {
                    if (setNonBlocking(clientSock) < 0) {
                        close(clientSock);
                        continue;
                    }
                    FD_SET(clientSock, &masterSet);
                    if (clientSock > maxFd) {
                        maxFd = clientSock;
                    }
                    buffers_[clientSock] = {};
                }
                continue;
            }

            auto& buffer = buffers_[fd];
            std::vector<char> chunk(kBufferSize);
            ssize_t bytesRead = recv(fd, chunk.data(), chunk.size(), 0);
            if (bytesRead < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                closeClient(fd, masterSet);
                if (fd == maxFd) {
                    while (maxFd >= 0 && !FD_ISSET(maxFd, &masterSet)) {
                        --maxFd;
                    }
                }
                continue;
            }
            if (bytesRead == 0) {
                closeClient(fd, masterSet);
                if (fd == maxFd) {
                    while (maxFd >= 0 && !FD_ISSET(maxFd, &masterSet)) {
                        --maxFd;
                    }
                }
                continue;
            }

            buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + bytesRead);

            HttpRequest maybeRequest;
            if (!HttpRequest::tryParse(buffer, maybeRequest)) {
                continue;
            }

            processRequest(fd, maybeRequest);
            closeClient(fd, masterSet);
            if (fd == maxFd) {
                while (maxFd >= 0 && !FD_ISSET(maxFd, &masterSet)) {
                    --maxFd;
                }
            }
        }
    }
}

void Server::closeClient(int clientSock, fd_set& masterSet) {
    close(clientSock);
    FD_CLR(clientSock, &masterSet);
    buffers_.erase(clientSock);
}

void Server::processRequest(int clientSock, const HttpRequest& request) {
    HttpResponse response;
    response.setHeader("Connection", "close");

    switch (request.method()) {
    case HttpRequest::Method::Get:
    case HttpRequest::Method::Head: {
        std::string path = request.uri();
        const auto queryPos = path.find('?');
        if (queryPos != std::string::npos) {
            path = path.substr(0, queryPos);
        }
        if (path.empty() || path == "/") {
            path = "index.html";
        } else if (path.front() == '/') {
            path.erase(path.begin());
        }

        std::vector<char> data;
        if (!fileHandler_.readFile(path, data)) {
            response.setStatus(404, "Not Found");
            const std::string body = "404 Not Found";
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Content-Length", std::to_string(body.size()));
            if (request.method() != HttpRequest::Method::Head) {
                response.setBody(body);
            }
            respond(clientSock, response);
            return;
        }

        const std::string mime = fileHandler_.detectMimeType(path);
        response.setStatus(200, "OK");
        response.setHeader("Content-Type", mime);
        response.setHeader("Content-Length", std::to_string(data.size()));
        if (request.method() != HttpRequest::Method::Head) {
            response.setBody(data);
        }
        respond(clientSock, response);
        return;
    }
    case HttpRequest::Method::Post: {
        if (request.uri() != "/") {
            response.setStatus(404, "Not Found");
            const std::string body = "Endpoint not found";
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Content-Length", std::to_string(body.size()));
            response.setBody(body);
            respond(clientSock, response);
            return;
        }

        const std::string contentType = request.headerValue("content-type");
        auto result = uploadHandler_.handle(request.body(), contentType);
        if (!result.success) {
            response.setStatus(400, "Bad Request");
        } else {
            response.setStatus(200, "OK");
        }
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Content-Length", std::to_string(result.message.size()));
        response.setBody(result.message);
        respond(clientSock, response);
        return;
    }
    default:
        response.setStatus(405, "Method Not Allowed");
        response.setHeader("Allow", "GET, HEAD, POST");
        {
            const std::string body = "Method not allowed";
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Content-Length", std::to_string(body.size()));
            response.setBody(body);
        }
        respond(clientSock, response);
        return;
    }
}

void Server::respond(int clientSock, const HttpResponse& response) {
    const std::vector<char> data = response.serialize();
    size_t totalSent = 0;

    while (totalSent < data.size()) {
        ssize_t sent = send(clientSock, data.data() + totalSent, data.size() - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                fd_set writeSet;
                FD_ZERO(&writeSet);
                FD_SET(clientSock, &writeSet);
                if (select(clientSock + 1, nullptr, &writeSet, nullptr, nullptr) <= 0) {
                    break;
                }
                continue;
            }
            break;
        }
        totalSent += static_cast<size_t>(sent);
    }
}
