#include "Server.h"

#include "FileHandler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "UploadHandler.h"

#include <algorithm>
#include <arpa/inet.h>
#include <csignal>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr std::size_t kReadBufferSize = 8192;

std::size_t findHeaderBoundary(const std::vector<char>& buffer, std::size_t& delimiterLength) {
    std::string data(buffer.begin(), buffer.end());
    std::size_t pos = data.find("\r\n\r\n");
    if (pos != std::string::npos) {
        delimiterLength = 4;
        return pos;
    }
    pos = data.find("\n\n");
    if (pos != std::string::npos) {
        delimiterLength = 2;
        return pos;
    }
    delimiterLength = 0;
    return std::string::npos;
}

std::size_t parseContentLength(const std::string& headers) {
    constexpr char kHeaderName[] = "content-length:";
    std::string lowered = headers;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    std::size_t pos = lowered.find(kHeaderName);
    if (pos == std::string::npos) {
        return 0;
    }

    pos += sizeof(kHeaderName) - 1;
    while (pos < headers.size() && std::isspace(static_cast<unsigned char>(headers[pos]))) {
        ++pos;
    }

    std::size_t end = pos;
    while (end < headers.size() && std::isdigit(static_cast<unsigned char>(headers[end]))) {
        ++end;
    }

    if (end == pos) {
        return 0;
    }

    return static_cast<std::size_t>(std::stoul(headers.substr(pos, end - pos)));
}

std::string parseMethod(const std::string& requestLine) {
    std::size_t space = requestLine.find(' ');
    if (space == std::string::npos) {
        return {};
    }
    return requestLine.substr(0, space);
}

} // namespace

Server::Server(int port)
    : port_(port),
      serverSock_(-1),
      running_(false),
      fileHandler_(std::make_unique<FileHandler>()),
      uploadHandler_(std::make_unique<UploadHandler>()) {}

Server::~Server() {
    cleanup();
}

void Server::run() {
    std::signal(SIGPIPE, SIG_IGN);
    initSocket();
    running_ = true;

    while (running_) {
        int ready = ::poll(pollFds_.data(), pollFds_.size(), -1);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error(std::string("poll failed: ") + std::strerror(errno));
        }

        for (std::size_t index = 0; index < pollFds_.size();) {
            pollfd entry = pollFds_[index];
            short revents = entry.revents;
            pollFds_[index].revents = 0;

            if (entry.fd == serverSock_) {
                if (revents & POLLIN) {
                    while (true) {
                        sockaddr_in clientAddr {};
                        socklen_t addrLen = sizeof(clientAddr);
                        int client = ::accept(serverSock_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
                        if (client < 0) {
                            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                                break;
                            }
                            std::cerr << "accept failed: " << std::strerror(errno) << std::endl;
                            break;
                        }

                        int flags = ::fcntl(client, F_GETFL, 0);
                        ::fcntl(client, F_SETFL, flags | O_NONBLOCK);

                        pollfd clientFd {};
                        clientFd.fd = client;
                        clientFd.events = POLLIN;
                        pollFds_.push_back(clientFd);
                    }
                }
                ++index;
                continue;
            }

            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                closeClient(entry.fd);
                continue;
            }

            if (revents & POLLIN) {
                bool closed = handleClient(entry.fd);
                if (closed) {
                    continue;
                }
            }

            ++index;
        }
    }
}

void Server::initSocket() {
    serverSock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error("Unable to create socket");
    }

    int enable = 1;
    ::setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    ::setsockopt(serverSock_, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

    int flags = ::fcntl(serverSock_, F_GETFL, 0);
    ::fcntl(serverSock_, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (::bind(serverSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("bind failed: ") + std::strerror(errno));
    }

    if (::listen(serverSock_, SOMAXCONN) < 0) {
        throw std::runtime_error(std::string("listen failed: ") + std::strerror(errno));
    }

    pollfd serverFd {};
    serverFd.fd = serverSock_;
    serverFd.events = POLLIN;
    pollFds_.push_back(serverFd);
}

void Server::cleanup() {
    if (serverSock_ >= 0) {
        ::close(serverSock_);
        serverSock_ = -1;
    }
    for (auto& [fd, _] : clients_) {
        ::close(fd);
    }
    clients_.clear();
    pollFds_.clear();
}

bool Server::handleClient(int clientSock) {
    bool connectionClosed = false;
    std::vector<char> rawRequest;

    try {
        rawRequest = readRequest(clientSock, connectionClosed);
    } catch (const std::exception& ex) {
        std::cerr << "Error reading request: " << ex.what() << std::endl;
        HttpResponse response(400, "Bad Request");
        response.setHeader("Content-Type", "text/plain");
        response.setBody("Malformed HTTP request");
        sendResponse(clientSock, response);
        connectionClosed = true;
    }

    if (connectionClosed) {
        closeClient(clientSock);
        return true;
    }

    if (rawRequest.empty()) {
        return false;
    }

    processRequest(clientSock, rawRequest);
    closeClient(clientSock);
    return true;
}

void Server::closeClient(int clientSock) {
    ::close(clientSock);
    clients_.erase(clientSock);

    for (std::size_t i = 0; i < pollFds_.size(); ++i) {
        if (pollFds_[i].fd == clientSock) {
            pollFds_.erase(pollFds_.begin() + static_cast<long>(i));
            break;
        }
    }
}

std::vector<char> Server::readRequest(int clientSock, bool& connectionClosed) {
    ClientContext& ctx = clients_[clientSock];

    char buffer[kReadBufferSize];
    while (true) {
        ssize_t bytesRead = ::recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            ctx.buffer.insert(ctx.buffer.end(), buffer, buffer + bytesRead);
            if (static_cast<std::size_t>(bytesRead) < sizeof(buffer)) {
                break;
            }
        } else if (bytesRead == 0) {
            connectionClosed = true;
            return {};
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            connectionClosed = true;
            return {};
        }
    }

    if (!ctx.headerParsed) {
        std::size_t delimiterLength = 0;
        std::size_t headerPos = findHeaderBoundary(ctx.buffer, delimiterLength);
        if (headerPos == std::string::npos) {
            return {};
        }

        std::string headerSection(ctx.buffer.begin(), ctx.buffer.begin() + headerPos);
        std::size_t lineEnd = headerSection.find_first_of("\r\n");
        std::string requestLine = headerSection.substr(0, lineEnd);
        ctx.method = parseMethod(requestLine);

        ctx.headerParsed = true;
        ctx.headerEnd = headerPos + delimiterLength;

        ctx.contentLength = parseContentLength(headerSection);
        if (ctx.method == "GET" || ctx.method == "HEAD") {
            ctx.contentLength = 0;
        }
    }

    std::size_t totalExpected = ctx.headerEnd + ctx.contentLength;
    if (ctx.buffer.size() < totalExpected) {
        return {};
    }

    std::vector<char> request(ctx.buffer.begin(), ctx.buffer.begin() + static_cast<long>(totalExpected));
    ctx.buffer.erase(ctx.buffer.begin(), ctx.buffer.begin() + static_cast<long>(totalExpected));
    ctx.headerParsed = false;
    ctx.headerEnd = 0;
    ctx.contentLength = 0;
    ctx.method.clear();

    return request;
}

void Server::processRequest(int clientSock, const std::vector<char>& rawRequest) {
    HttpResponse response;
    try {
        HttpRequest request = HttpRequest::parse(rawRequest);
        const std::string method = request.method();
        HttpResponse result;

        if (method == "GET") {
            result = fileHandler_->serveFile("public", request.path());
        } else if (method == "HEAD") {
            result = fileHandler_->serveFile("public", request.path());
            std::size_t payloadSize = result.body().size();
            result.setHeader("Content-Length", std::to_string(payloadSize));
            result.setBody(std::vector<char>{});
        } else if (method == "POST") {
            const std::string contentType = request.headerValue("content-type");
            result = uploadHandler_->handle(request.body(), contentType);
        } else {
            result.setStatus(405, "Method Not Allowed");
            result.setHeader("Content-Type", "text/plain");
            result.setBody("405 Method Not Allowed");
        }

        result.setHeader("Connection", "close");
        response = result;
    } catch (const std::exception& ex) {
        std::cerr << "Request processing failed: " << ex.what() << std::endl;
        response.setStatus(500, "Internal Server Error");
        response.setHeader("Content-Type", "text/plain");
        response.setBody("Internal Server Error");
        response.setHeader("Connection", "close");
    }

    sendResponse(clientSock, response);
}

void Server::sendResponse(int clientSock, const HttpResponse& response) {
    std::vector<char> bytes = response.serialize();

    std::size_t totalSent = 0;
    while (totalSent < bytes.size()) {
        ssize_t sent = ::send(clientSock,
                              bytes.data() + static_cast<long>(totalSent),
                              static_cast<int>(bytes.size() - totalSent),
#ifdef MSG_NOSIGNAL
                              MSG_NOSIGNAL
#else
                              0
#endif
        );

        if (sent < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            break;
        }

        totalSent += static_cast<std::size_t>(sent);
    }
}
