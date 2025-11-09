#include "Server.h"

#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <sstream>
#include <stdexcept>

namespace {

constexpr int kBacklog = 16;
constexpr int kBufferSize = 4096;

std::pair<size_t, size_t> locateHeaderDelimiter(const std::vector<char>& data) {
    const std::string delimiterCRLF = "\r\n\r\n";
    const std::string delimiterLF = "\n\n";

    for (size_t i = 0; i + delimiterCRLF.size() <= data.size(); ++i) {
        if (std::equal(delimiterCRLF.begin(), delimiterCRLF.end(), data.begin() + static_cast<std::ptrdiff_t>(i))) {
            return {i, delimiterCRLF.size()};
        }
    }
    for (size_t i = 0; i + delimiterLF.size() <= data.size(); ++i) {
        if (std::equal(delimiterLF.begin(), delimiterLF.end(), data.begin() + static_cast<std::ptrdiff_t>(i))) {
            return {i, delimiterLF.size()};
        }
    }
    return {std::string::npos, 0};
}

std::string trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

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
      serverFd_(-1),
      fileHandler_("public") {}

void Server::run() {
    std::signal(SIGPIPE, SIG_IGN);
    setupSocket();
    eventLoop();
}

void Server::setupSocket() {
    serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int enable = 1;
    if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        ::close(serverFd_);
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }

    if (setNonBlocking(serverFd_) == -1) {
        ::close(serverFd_);
        throw std::runtime_error("Failed to set non-blocking mode");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        ::close(serverFd_);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd_, kBacklog) < 0) {
        ::close(serverFd_);
        throw std::runtime_error("Failed to listen on socket");
    }
}

void Server::eventLoop() {
    std::vector<pollfd> pollFds;
    pollFds.push_back(pollfd{serverFd_, POLLIN, 0});

    while (true) {
        int ready = ::poll(pollFds.data(), static_cast<nfds_t>(pollFds.size()), -1);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("poll failed");
        }

        if (pollFds[0].revents & POLLIN) {
            acceptConnections(pollFds);
        }

        for (size_t i = 1; i < pollFds.size();) {
            if (pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                closeClient(pollFds, i);
                continue;
            }

            if (pollFds[i].revents & POLLIN) {
                handleClient(pollFds, i);
                continue;
            }

            ++i;
        }
    }
}

void Server::acceptConnections(std::vector<pollfd>& pollFds) {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = ::accept(serverFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            continue;
        }

        if (setNonBlocking(clientFd) == -1) {
            ::close(clientFd);
            continue;
        }

        pollFds.push_back(pollfd{clientFd, POLLIN, 0});
        clients_.emplace(clientFd, ClientConnection{});
    }
}

void Server::handleClient(std::vector<pollfd>& pollFds, size_t index) {
    pollfd& descriptor = pollFds[index];
    int clientFd = descriptor.fd;
    auto clientIt = clients_.find(clientFd);
    if (clientIt == clients_.end()) {
        closeClient(pollFds, index);
        return;
    }
    ClientConnection& client = clientIt->second;

    bool connectionAlive = true;
    while (connectionAlive) {
        char buffer[kBufferSize];
        const ssize_t bytesRead = ::recv(clientFd, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            client.buffer.insert(client.buffer.end(), buffer, buffer + bytesRead);
            if (evaluateRequestBoundary(client)) {
                processBuffer(clientFd, client);
                closeClient(pollFds, index);
                connectionAlive = false;
            }
        } else if (bytesRead == 0) {
            closeClient(pollFds, index);
            connectionAlive = false;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            closeClient(pollFds, index);
            connectionAlive = false;
        }
    }

}

bool Server::evaluateRequestBoundary(ClientConnection& client) {
    if (client.headerParsed) {
        if (client.expectedSize == 0) {
            return true;
        }
        return client.buffer.size() >= client.expectedSize;
    }

    const auto [headerEnd, delimiterLen] = locateHeaderDelimiter(client.buffer);
    if (headerEnd == std::string::npos) {
        return false;
    }

    const size_t headerLength = headerEnd;
    const size_t totalHeaderSize = headerLength + delimiterLen;
    std::string headerSection(client.buffer.data(), headerLength);
    std::istringstream headerStream(headerSection);

    std::string requestLine;
    if (!std::getline(headerStream, requestLine)) {
        client.headerParsed = true;
        client.expectedSize = totalHeaderSize;
        return client.buffer.size() >= client.expectedSize;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }
    std::istringstream requestLineStream(requestLine);
    std::string method;
    requestLineStream >> method;
    method = toUpper(method);

    size_t contentLength = 0;
    bool hasContentLength = false;

    std::string headerLine;
    while (std::getline(headerStream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        const auto colonPos = headerLine.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        std::string key = trim(headerLine.substr(0, colonPos));
        std::string value = trim(headerLine.substr(colonPos + 1));
        if (toUpper(key) == "CONTENT-LENGTH") {
            try {
                contentLength = std::stoul(value);
                hasContentLength = true;
            } catch (...) {
                contentLength = 0;
            }
        }
    }

    bool expectsBody = (method == "POST" || method == "PUT" || method == "PATCH");
    if (!expectsBody) {
        contentLength = 0;
    } else if (!hasContentLength) {
        contentLength = 0;
    }

    client.headerParsed = true;
    client.expectedSize = totalHeaderSize + contentLength;
    return client.buffer.size() >= client.expectedSize;
}

void Server::processBuffer(int clientFd, ClientConnection& client) {
    HttpRequest request;
    if (!request.parse(client.buffer)) {
        respondWithStatus(clientFd, 400, "Bad Request", "400 Bad Request");
        return;
    }

    HttpResponse response;
    const std::string methodUpper = toUpper(request.method());
    const std::string& path = request.path();

    if (methodUpper == "GET" || methodUpper == "HEAD") {
        std::vector<char> data;
        std::string mime;
        if (!fileHandler_.readFile(path, data, mime)) {
            respondWithStatus(clientFd, 404, "Not Found", "404 Not Found");
            return;
        }

        response.setHeader("Content-Type", mime);
        if (methodUpper == "HEAD") {
            response.setHeader("Content-Length", std::to_string(data.size()));
            response.setBody(std::vector<char>{});
        } else {
            response.setBody(std::move(data));
        }
        sendResponse(clientFd, response);
        return;
    }

    if (methodUpper == "POST" && path == "/") {
        if (!request.hasHeader("Content-Type")) {
            respondWithStatus(clientFd, 400, "Bad Request", "400 Bad Request: missing Content-Type");
            return;
        }
        try {
            const auto result = uploadHandler_.handle(request.body(), request.headerValue("Content-Type"));
            const std::string reason = result.reason.empty()
                                           ? (result.success ? "OK" : "Error")
                                           : result.reason;
            response.setStatus(result.statusCode, reason);
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody(result.message);
            sendResponse(clientFd, response);
        } catch (const std::exception&) {
            respondWithStatus(clientFd, 500, "Internal Server Error", "500 Internal Server Error");
        }
        return;
    }

    if (methodUpper == "POST") {
        respondWithStatus(clientFd, 404, "Not Found", "404 Not Found");
        return;
    }

    HttpResponse methodError(405, "Method Not Allowed");
    methodError.setHeader("Allow", "GET, HEAD, POST");
    methodError.setHeader("Content-Type", "text/plain; charset=utf-8");
    methodError.setBody("405 Method Not Allowed");
    sendResponse(clientFd, methodError);
}

void Server::closeClient(std::vector<pollfd>& pollFds, size_t index) {
    const int fd = pollFds[index].fd;
    ::close(fd);
    clients_.erase(fd);
    pollFds.erase(pollFds.begin() + static_cast<std::ptrdiff_t>(index));
}

void Server::sendResponse(int clientFd, const HttpResponse& response) {
    const std::vector<char> bytes = response.toBytes();
    size_t totalSent = 0;
    while (totalSent < bytes.size()) {
        ssize_t sent = ::send(clientFd, bytes.data() + static_cast<std::ptrdiff_t>(totalSent),
                              bytes.size() - totalSent,
#ifdef MSG_NOSIGNAL
                              MSG_NOSIGNAL
#else
                              0
#endif
        );
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            break;
        }
        totalSent += static_cast<size_t>(sent);
    }
}

void Server::respondWithStatus(int clientFd, int status, const std::string& reason, const std::string& body) {
    HttpResponse response(status, reason);
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody(body);
    sendResponse(clientFd, response);
}
