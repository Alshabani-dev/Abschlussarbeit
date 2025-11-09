#include "Server.h"

#include <algorithm>
#include <arpa/inet.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

void setNonBlocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("fcntl(F_GETFL) failed");
    }
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("fcntl(F_SETFL) failed");
    }
}

std::string statusMessage(int code) {
    switch (code) {
    case 200:
        return "OK";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 415:
        return "Unsupported Media Type";
    case 500:
        return "Internal Server Error";
    default:
        return "HTTP Response";
    }
}

std::string escapeJson(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '\"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

} // namespace

Server::Server(int port)
    : port_(port),
      serverSock_(-1),
      running_(false),
      projectRoot_(std::filesystem::current_path()),
      publicRoot_(projectRoot_ / "public"),
      dataRoot_(projectRoot_ / "Data"),
      fileHandler_(),
      uploadHandler_() {
    fileHandler_.setPublicRoot(publicRoot_);
    uploadHandler_.setDataRoot(dataRoot_);
    std::filesystem::create_directories(publicRoot_);
    std::filesystem::create_directories(dataRoot_);
}

Server::~Server() {
    if (serverSock_ >= 0) {
        ::close(serverSock_);
        serverSock_ = -1;
    }
}

void Server::run() {
    std::signal(SIGPIPE, SIG_IGN);
    setupServerSocket();
    running_ = true;
    mainLoop();
}

void Server::setupServerSocket() {
    serverSock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (::setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }

    setNonBlocking(serverSock_);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (::bind(serverSock_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("bind() failed");
    }

    if (::listen(serverSock_, SOMAXCONN) < 0) {
        throw std::runtime_error("listen() failed");
    }

    std::cout << "Server listening on port " << port_ << std::endl;
}

void Server::mainLoop() {
    std::vector<pollfd> pollFds;
    pollFds.push_back(pollfd{serverSock_, POLLIN, 0});

    while (running_) {
        const int ready = ::poll(pollFds.data(), static_cast<nfds_t>(pollFds.size()), -1);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("poll() failed");
        }

        std::size_t index = 0;
        while (index < pollFds.size()) {
            const int fd = pollFds[index].fd;
            const short revents = pollFds[index].revents;

            if (fd == -1) {
                ++index;
                continue;
            }

            if (!(revents & (POLLIN | POLLERR | POLLHUP))) {
                ++index;
                continue;
            }

            if (fd == serverSock_) {
                handleNewConnection(pollFds);
            } else {
                if (revents & (POLLERR | POLLHUP)) {
                    closeClient(fd);
                    pollFds[index].fd = -1;
                } else {
                    handleClient(fd);
                }
            }
            ++index;
        }

        pollFds.erase(std::remove_if(pollFds.begin(), pollFds.end(), [](const pollfd& entry) {
            return entry.fd == -1;
        }), pollFds.end());
    }
}

void Server::handleNewConnection(std::vector<pollfd>& pollFds) {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        const int clientSock = ::accept(serverSock_, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSock < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            std::perror("accept");
            break;
        }

        try {
            setNonBlocking(clientSock);
        } catch (const std::exception& ex) {
            std::cerr << "Failed to set client socket non-blocking: " << ex.what() << std::endl;
            ::close(clientSock);
            continue;
        }

        pollfd clientFd{};
        clientFd.fd = clientSock;
        clientFd.events = POLLIN;
        pollFds.push_back(clientFd);
        clientBuffers_[clientSock] = {};
    }
}

void Server::handleClient(int clientSock) {
    std::vector<char>& buffer = clientBuffers_[clientSock];
    char readBuffer[8192];

    while (true) {
        const ssize_t bytesRead = ::recv(clientSock, readBuffer, sizeof(readBuffer), 0);
        if (bytesRead > 0) {
            buffer.insert(buffer.end(), readBuffer, readBuffer + bytesRead);
        } else if (bytesRead == 0) {
            closeClient(clientSock);
            return;
        } else {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            closeClient(clientSock);
            return;
        }
    }

    while (!buffer.empty()) {
        HttpRequest request;
        std::size_t consumed = 0;
        if (!HttpRequest::tryParse(buffer, request, consumed)) {
            break;
        }

        HttpResponse response;
        try {
            response = routeRequest(request);
        } catch (const std::exception& ex) {
            std::cerr << "Error processing request: " << ex.what() << std::endl;
            response.setStatus(500, statusMessage(500));
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody("Internal Server Error");
        }

        sendResponse(clientSock, response);

        if (consumed >= buffer.size()) {
            buffer.clear();
        } else {
            buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(consumed));
        }

        closeClient(clientSock);
        break;
    }
}

void Server::closeClient(int clientSock) {
    clientBuffers_.erase(clientSock);
    ::close(clientSock);
}

void Server::sendResponse(int clientSock, const HttpResponse& response) {
    const std::vector<char> payload = response.serialize();
    std::size_t totalSent = 0;

    while (totalSent < payload.size()) {
#ifdef MSG_NOSIGNAL
        const ssize_t sent = ::send(clientSock, payload.data() + totalSent, payload.size() - totalSent, MSG_NOSIGNAL);
#else
        const ssize_t sent = ::send(clientSock, payload.data() + totalSent, payload.size() - totalSent, 0);
#endif
        if (sent < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        totalSent += static_cast<std::size_t>(sent);
    }
}

HttpResponse Server::routeRequest(const HttpRequest& request) {
    HttpResponse response;
    const std::string method = request.method();
    std::string path = request.path();

    const auto queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);
    }

    if (method == "GET" || method == "HEAD") {
        bool served = false;
        try {
            served = fileHandler_.serve(path, response);
        } catch (const std::exception& ex) {
            std::cerr << "Static file error: " << ex.what() << std::endl;
            response.setStatus(500, statusMessage(500));
            response.setHeader("Content-Type", "text/plain; charset=utf-8");
            response.setBody("Internal Server Error");
            return response;
        }

        if (!served) {
            response.setStatus(404, statusMessage(404));
            response.setHeader("Content-Type", "text/html; charset=utf-8");
            response.setBody("<h1>404 Not Found</h1>");
        } else if (method == "HEAD") {
            response.setBody(std::vector<char>{});
        }

        return response;
    }

    if (method == "POST" && path == "/") {
        const std::string contentType = request.headerValue("Content-Type");
        try {
            const UploadResult upload = uploadHandler_.handle(request.body(), contentType);
            response.setStatus(upload.statusCode, statusMessage(upload.statusCode));
            response.setHeader("Content-Type", "application/json; charset=utf-8");
            std::ostringstream json;
            json << "{"
                 << "\"message\":\"" << escapeJson(upload.message) << "\"";
            if (upload.statusCode == 200 && !upload.storedFilename.empty()) {
                json << ",\"filename\":\"" << escapeJson(upload.storedFilename) << "\"";
            }
            json << "}";
            response.setBody(json.str());
        } catch (const std::exception& ex) {
            response.setStatus(500, statusMessage(500));
            response.setHeader("Content-Type", "application/json; charset=utf-8");
            std::ostringstream json;
            json << "{\"message\":\"" << escapeJson(ex.what()) << "\"}";
            response.setBody(json.str());
        }
        return response;
    }

    response.setStatus(405, statusMessage(405));
    response.setHeader("Allow", "GET, HEAD, POST");
    response.setHeader("Content-Type", "text/plain; charset=utf-8");
    response.setBody("Method Not Allowed");
    return response;
}
