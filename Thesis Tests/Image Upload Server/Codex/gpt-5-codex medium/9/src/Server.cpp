#include "Server.h"

#include "FileHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UploadHandler.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace {

constexpr int kBacklog = 16;
constexpr int kReadBufferSize = 4096;

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

std::string statusToReason(int statusCode) {
    switch (statusCode) {
    case 200:
        return "OK";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 413:
        return "Payload Too Large";
    case 415:
        return "Unsupported Media Type";
    case 500:
        return "Internal Server Error";
    default:
        return statusCode >= 500 ? "Internal Server Error" : "OK";
    }
}

} // namespace

Server::Server(int port)
    : port_(port),
      publicDir_("public"),
      uploadDir_("Data"),
      fileHandler_(std::make_unique<FileHandler>(publicDir_)),
      uploadHandler_(std::make_unique<UploadHandler>(uploadDir_)) {}

Server::~Server() = default;

void Server::run() {
    setupSignalHandling();
    const int listenSock = createListeningSocket();
    eventLoop(listenSock);
    ::close(listenSock);
}

void Server::setupSignalHandling() const {
    std::signal(SIGPIPE, SIG_IGN);
}

int Server::createListeningSocket() const {
    const int listenSock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == -1) {
        throw std::runtime_error(std::string("socket failed: ") + std::strerror(errno));
    }

    int opt = 1;
    if (::setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        ::close(listenSock);
        throw std::runtime_error(std::string("setsockopt failed: ") + std::strerror(errno));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (::bind(listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        ::close(listenSock);
        throw std::runtime_error(std::string("bind failed: ") + std::strerror(errno));
    }

    if (::listen(listenSock, kBacklog) == -1) {
        ::close(listenSock);
        throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));
    }

    if (setNonBlocking(listenSock) == -1) {
        ::close(listenSock);
        throw std::runtime_error(std::string("fcntl failed: ") + std::strerror(errno));
    }

    std::cout << "Listening on port " << port_ << '\n';
    return listenSock;
}

void Server::eventLoop(int listenSock) {
    std::unordered_map<int, ClientState> clients;

    while (true) {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(listenSock, &readFds);
        int maxFd = listenSock;

        for (const auto& [fd, state] : clients) {
            if (!state.closed) {
                FD_SET(fd, &readFds);
                if (fd > maxFd) {
                    maxFd = fd;
                }
            }
        }

        const int ready = ::select(maxFd + 1, &readFds, nullptr, nullptr, nullptr);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("select failed: ") + std::strerror(errno));
        }

        if (FD_ISSET(listenSock, &readFds)) {
            acceptClients(listenSock, clients);
        }

        for (auto& [fd, state] : clients) {
            if (state.closed) {
                continue;
            }
            if (FD_ISSET(fd, &readFds)) {
                handleClientData(fd, state);
            }
        }

        for (auto it = clients.begin(); it != clients.end();) {
            if (it->second.closed) {
                ::close(it->first);
                it = clients.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void Server::acceptClients(int listenSock, std::unordered_map<int, ClientState>& clients) {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        const int clientSock =
            ::accept(listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSock == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "accept failed: " << std::strerror(errno) << '\n';
            break;
        }

        if (setNonBlocking(clientSock) == -1) {
            std::cerr << "failed to set client socket non-blocking\n";
            ::close(clientSock);
            continue;
        }

        clients.emplace(clientSock, ClientState{});
    }
}

void Server::handleClientData(int clientSock, ClientState& state) {
    bool dataReceived = false;
    char buffer[kReadBufferSize];
    while (true) {
        const ssize_t bytesRead = ::recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            dataReceived = true;
            state.buffer.insert(state.buffer.end(), buffer, buffer + bytesRead);
        } else if (bytesRead == 0) {
            state.closed = true;
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            std::cerr << "recv failed: " << std::strerror(errno) << '\n';
            state.closed = true;
            return;
        }
    }

    if (dataReceived) {
        processRequest(clientSock, state);
    }
}

void Server::processRequest(int clientSock, ClientState& state) {
    HttpRequest request;
    const auto result = request.parse(state.buffer);
    if (result == HttpRequest::ParseResult::Incomplete) {
        return;
    }

    HttpResponse response;
    if (result == HttpRequest::ParseResult::Error) {
        response.setStatus(400, "Bad Request");
        std::string body = "Bad Request: " + request.error() + "\n";
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setBody(std::vector<char>(body.begin(), body.end()));
        sendResponse(clientSock, response);
        state.closed = true;
        state.buffer.clear();
        return;
    }

    const std::string method = request.method();
    const std::string path = request.path();

    if (method == "GET" || method == "HEAD") {
        std::string resource = path;
        if (resource == "/") {
            resource = "/index.html";
        }

        if (!fileHandler_->serveFile(resource, response)) {
            response.setStatus(404, "Not Found");
            const std::string message = "404 Not Found\n";
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody(std::vector<char>(message.begin(), message.end()));
        } else if (method == "HEAD") {
            const size_t contentLength = response.bodySize();
            response.setHeader("Content-Length", std::to_string(contentLength));
            response.setBody({});
        }
    } else if (method == "POST" && path == "/") {
        const auto headers = request.headers();
        auto contentTypeIt = headers.find("content-type");
        if (contentTypeIt == headers.end()) {
            response.setStatus(400, "Bad Request");
            const std::string message = "Missing Content-Type header\n";
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody(std::vector<char>(message.begin(), message.end()));
        } else {
            const UploadResult result =
                uploadHandler_->handle(request.body(), contentTypeIt->second);
            response.setStatus(result.statusCode, statusToReason(result.statusCode));
            std::string body = result.message;
            if (body.empty() && result.statusCode == 200) {
                body = "Upload successful";
            }
            body.push_back('\n');
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody(std::vector<char>(body.begin(), body.end()));
        }
    } else {
        response.setStatus(405, "Method Not Allowed");
        const std::string message = "Method Not Allowed\n";
        response.setHeader("Content-Type", "text/plain; charset=utf-8");
        response.setHeader("Allow", "GET, HEAD, POST");
        response.setBody(std::vector<char>(message.begin(), message.end()));
    }

    sendResponse(clientSock, response);
    state.closed = true;

    if (request.isComplete()) {
        const std::string delimiter1 = "\r\n\r\n";
        const std::string delimiter2 = "\n\n";

        auto it = std::search(state.buffer.begin(), state.buffer.end(), delimiter1.begin(),
                              delimiter1.end());
        size_t headerEnd = std::distance(state.buffer.begin(), it);
        size_t delimiterLength = delimiter1.size();
        if (it == state.buffer.end()) {
            it = std::search(state.buffer.begin(), state.buffer.end(), delimiter2.begin(),
                             delimiter2.end());
            headerEnd = std::distance(state.buffer.begin(), it);
            delimiterLength = delimiter2.size();
        }
        size_t consumed = headerEnd + delimiterLength + request.body().size();
        if (consumed <= state.buffer.size()) {
            state.buffer.erase(state.buffer.begin(),
                               state.buffer.begin() + static_cast<std::ptrdiff_t>(consumed));
        } else {
            state.buffer.clear();
        }
    }
}

void Server::sendResponse(int clientSock, const HttpResponse& response) {
    const std::vector<char> payload = response.serialize();
    size_t totalSent = 0;
    while (totalSent < payload.size()) {
        const ssize_t bytesSent =
            ::send(clientSock, payload.data() + totalSent, payload.size() - totalSent, MSG_NOSIGNAL);
        if (bytesSent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            std::cerr << "send failed: " << std::strerror(errno) << '\n';
            break;
        }
        totalSent += static_cast<size_t>(bytesSent);
    }
}
