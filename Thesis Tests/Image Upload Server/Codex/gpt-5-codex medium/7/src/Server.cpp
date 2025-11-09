#include "Server.h"

#include "HttpRequest.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr std::size_t MAX_REQUEST_SIZE = 10 * 1024 * 1024; // 10 MiB upper bound.

void trim(std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        value.clear();
        return;
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    value = value.substr(first, last - first + 1);
}

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
} // namespace

Server::Server(int port, std::string publicDirectory, std::string uploadDirectory)
    : port_(port),
      listenFd_(-1),
      publicDirectory_(std::move(publicDirectory)),
      uploadHandler_(std::move(uploadDirectory)),
      fileHandler_(publicDirectory_) {}

Server::~Server() {
    if (listenFd_ >= 0) {
        close(listenFd_);
    }
    for (auto& [fd, state] : clients_) {
        (void)state;
        close(fd);
    }
}

void Server::setupSocket() {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        throw std::runtime_error(std::string("Failed to create socket: ") + std::strerror(errno));
    }

    int opt = 1;
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error(std::string("Failed to set socket options: ") + std::strerror(errno));
    }

    if (setNonBlocking(listenFd_) == -1) {
        throw std::runtime_error(std::string("Failed to set socket non-blocking: ") + std::strerror(errno));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error(std::string("Failed to bind socket: ") + std::strerror(errno));
    }

    if (listen(listenFd_, SOMAXCONN) < 0) {
        throw std::runtime_error(std::string("Failed to listen on socket: ") + std::strerror(errno));
    }

    std::cout << "Server listening on port " << port_ << std::endl;
}

void Server::run() {
    setupSocket();

    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenFd_, &readSet);
        int maxFd = listenFd_;

        for (const auto& entry : clients_) {
            FD_SET(entry.first, &readSet);
            if (entry.first > maxFd) {
                maxFd = entry.first;
            }
        }

        const int ready = select(maxFd + 1, &readSet, nullptr, nullptr, nullptr);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("select() failed: " + std::string(std::strerror(errno)));
        }

        if (FD_ISSET(listenFd_, &readSet)) {
            acceptClient();
        }

        std::vector<int> readyClients;
        readyClients.reserve(clients_.size());
        for (const auto& entry : clients_) {
            const int clientFd = entry.first;
            if (FD_ISSET(clientFd, &readSet)) {
                readyClients.push_back(clientFd);
            }
        }

        for (int clientFd : readyClients) {
            handleClient(clientFd);
        }
    }
}

void Server::acceptClient() {
    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);
        const int clientFd = accept(listenFd_, reinterpret_cast<sockaddr*>(&clientAddress), &clientLen);
        if (clientFd < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            std::cerr << "accept() failed: " << std::strerror(errno) << std::endl;
            break;
        }

        if (setNonBlocking(clientFd) == -1) {
            std::cerr << "Failed to set client socket non-blocking" << std::endl;
            close(clientFd);
            continue;
        }

        clients_.emplace(clientFd, ClientState{});
    }
}

void Server::handleClient(int clientFd) {
    auto clientIt = clients_.find(clientFd);
    if (clientIt == clients_.end()) {
        return;
    }

    ClientState& state = clientIt->second;

    char buffer[4096];
    while (true) {
        const ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            state.buffer.insert(state.buffer.end(), buffer, buffer + bytesRead);

            if (state.buffer.size() > MAX_REQUEST_SIZE) {
                sendErrorResponse(clientFd, 413, "Payload Too Large", "Request exceeds maximum allowed size");
                closeClient(clientFd);
                return;
            }
            continue;
        }

        if (bytesRead == 0) {
            closeClient(clientFd);
            return;
        }

        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            break;
        }

        std::cerr << "recv() failed: " << std::strerror(errno) << std::endl;
        closeClient(clientFd);
        return;
    }

    if (state.buffer.empty()) {
        return;
    }

    processClientBuffer(clientFd, state);
}

bool Server::processClientBuffer(int clientFd, ClientState& state) {
    if (!state.headersParsed) {
        std::size_t delimiterLength = 0;
        const auto headerOpt = HttpRequest::findHeaderBoundary(state.buffer, delimiterLength);
        if (!headerOpt.has_value()) {
            return false;
        }

        state.headerEnd = headerOpt.value();
        state.delimiterLength = delimiterLength;
        parseHeadersIfNeeded(state);
    }

    const std::size_t totalExpected = state.headerEnd + state.delimiterLength + state.expectedBodyLength;
    if (state.buffer.size() < totalExpected) {
        return false;
    }

    std::vector<char> rawRequest(state.buffer.begin(),
                                 state.buffer.begin() + static_cast<std::ptrdiff_t>(totalExpected));

    HttpRequest request;
    if (!request.parse(rawRequest)) {
        sendErrorResponse(clientFd, 400, "Bad Request", "Malformed HTTP request");
        closeClient(clientFd);
        return true;
    }

    try {
        HttpResponse response = routeRequest(request);
        sendResponse(clientFd, response);
    } catch (const std::exception& ex) {
        std::cerr << "Error while handling request: " << ex.what() << std::endl;
        sendErrorResponse(clientFd, 500, "Internal Server Error", "An internal error occurred");
    }

    closeClient(clientFd);
    return true;
}

void Server::parseHeadersIfNeeded(ClientState& state) {
    if (state.headersParsed) {
        return;
    }

    std::string headerBlock(
        state.buffer.begin(),
        state.buffer.begin() + static_cast<std::ptrdiff_t>(state.headerEnd));

    std::istringstream stream(headerBlock);
    std::string requestLine;
    if (std::getline(stream, requestLine)) {
        if (!requestLine.empty() && requestLine.back() == '\r') {
            requestLine.pop_back();
        }
        std::istringstream requestLineStream(requestLine);
        requestLineStream >> state.method;
    }

    std::string headerLine;
    std::size_t contentLength = 0;
    while (std::getline(stream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        if (headerLine.empty()) {
            continue;
        }

        const auto colonPos = headerLine.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        std::string key = headerLine.substr(0, colonPos);
        std::string value = headerLine.substr(colonPos + 1);
        trim(key);
        trim(value);

        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (lowerKey == "content-length") {
            try {
                contentLength = static_cast<std::size_t>(std::stoul(value));
            } catch (...) {
                contentLength = 0;
            }
        }
    }

    if (state.method == "GET" || state.method == "HEAD") {
        state.expectedBodyLength = 0;
    } else {
        state.expectedBodyLength = contentLength;
    }

    state.headersParsed = true;
}

HttpResponse Server::routeRequest(const HttpRequest& request) {
    const std::string& method = request.method();
    const std::string& path = request.path();

    if (method == "GET" || method == "HEAD") {
        HttpResponse response = fileHandler_.serve(path);
        // Basic HEAD handling: reuse GET response.
        if (method == "HEAD") {
            // Response already prepared; for simplicity we send the same payload.
        }
        return response;
    }

    if (method == "POST") {
        if (path == "/" || path == "/upload") {
            return uploadHandler_.handle(request);
        }
        HttpResponse response(404, "Not Found");
        response.setBody("Endpoint not found", "text/plain; charset=utf-8");
        return response;
    }

    HttpResponse response(405, "Method Not Allowed");
    response.setHeader("Allow", "GET, HEAD, POST");
    response.setBody("Method not allowed", "text/plain; charset=utf-8");
    return response;
}

void Server::sendResponse(int clientFd, const HttpResponse& response) {
    const std::vector<char> payload = response.serialize();
    std::size_t totalSent = 0;

    while (totalSent < payload.size()) {
        ssize_t sent = 0;
#ifdef MSG_NOSIGNAL
        sent = send(clientFd, payload.data() + totalSent,
                    payload.size() - totalSent, MSG_NOSIGNAL);
#else
        sent = send(clientFd, payload.data() + totalSent,
                    payload.size() - totalSent, 0);
#endif
        if (sent < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            std::cerr << "send() failed: " << std::strerror(errno) << std::endl;
            break;
        }
        totalSent += static_cast<std::size_t>(sent);
    }
}

void Server::sendErrorResponse(int clientFd, int statusCode, const std::string& statusMessage, const std::string& body) {
    HttpResponse response(statusCode, statusMessage);
    response.setBody(body, "text/plain; charset=utf-8");
    sendResponse(clientFd, response);
}

void Server::closeClient(int clientFd) {
    const auto it = clients_.find(clientFd);
    if (it != clients_.end()) {
        close(clientFd);
        clients_.erase(it);
    }
}
