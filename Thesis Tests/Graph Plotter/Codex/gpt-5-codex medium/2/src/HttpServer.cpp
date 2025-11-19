#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>

namespace {
constexpr int kBacklog = 16;
constexpr int kBufferSize = 4096;
}

HttpServer::HttpServer(int port)
    : port_(port), serverSocket_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    serverSocket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::perror("socket");
        return;
    }

    int opt = 1;
    ::setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (::bind(serverSocket_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        std::perror("bind");
        ::close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    if (::listen(serverSocket_, kBacklog) < 0) {
        std::perror("listen");
        ::close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    running_ = true;
    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = ::accept(serverSocket_, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("accept");
            break;
        }
        handleClient(clientSocket);
        ::close(clientSocket);
    }

    stop();
}

void HttpServer::stop() {
    running_ = false;
    if (serverSocket_ >= 0) {
        ::close(serverSocket_);
        serverSocket_ = -1;
    }
}

bool HttpServer::readRawRequest(int clientSocket, std::string &rawRequest) const {
    std::string data;
    data.reserve(2048);
    char buffer[kBufferSize];
    size_t contentLength = 0;
    size_t headerEnd = std::string::npos;

    while (true) {
        const ssize_t received = ::recv(clientSocket, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break;
        }
        data.append(buffer, received);

        if (headerEnd == std::string::npos) {
            headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                const std::string headersPart = data.substr(0, headerEnd);
                std::istringstream headerStream(headersPart);
                std::string line;
                while (std::getline(headerStream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    const size_t colonPos = line.find(':');
                    if (colonPos == std::string::npos) {
                        continue;
                    }
                    std::string key = Utils::trim(line.substr(0, colonPos));
                    if (key.empty()) {
                        continue;
                    }
                    std::string value = Utils::trim(line.substr(colonPos + 1));
                    std::string lowerKey = key;
                    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
                    if (lowerKey == "content-length") {
                        contentLength = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
                    }
                }
            }
        }

        if (headerEnd != std::string::npos) {
            const size_t headerLength = headerEnd + 4;
            const size_t bodyLength = data.size() - headerLength;
            if (bodyLength >= contentLength) {
                break;
            }
        }
    }

    rawRequest = data;
    return !rawRequest.empty();
}

HttpServer::HttpRequest HttpServer::parseRequest(const std::string &raw) const {
    HttpRequest request;
    std::istringstream stream(raw);
    std::string line;

    if (!std::getline(stream, line)) {
        return request;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    std::istringstream lineStream(line);
    lineStream >> request.method;
    lineStream >> request.path;

    while (std::getline(stream, line)) {
        if (line == "\r" || line == "") {
            break;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break;
        }
        auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        const std::string key = Utils::trim(line.substr(0, colon));
        const std::string value = Utils::trim(line.substr(colon + 1));
        request.headers[key] = value;
    }

    std::ostringstream bodyStream;
    bodyStream << stream.rdbuf();
    request.body = bodyStream.str();
    return request;
}

void HttpServer::handleClient(int clientSocket) {
    std::string rawRequest;
    if (!readRawRequest(clientSocket, rawRequest)) {
        return;
    }

    HttpRequest request = parseRequest(rawRequest);
    if (request.method == "GET") {
        handleGet(request, clientSocket);
    } else if (request.method == "POST") {
        handlePost(request, clientSocket);
    } else {
        sendTextResponse(clientSocket, 405, "Method Not Allowed", "text/plain",
                         "Only GET and POST are supported.\n");
    }
}

void HttpServer::handleGet(const HttpRequest &request, int clientSocket) {
    std::string path = request.path.empty() ? "/" : request.path;
    const auto queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        path = path.substr(0, queryPos);
    }
    if (path == "/") {
        path = "/index.html";
    }
    if (path.find("..") != std::string::npos) {
        sendTextResponse(clientSocket, 400, "Bad Request", "text/plain",
                         "Invalid path.\n");
        return;
    }

    const std::string content = Utils::readFileFromCandidates({"public" + path, "../public" + path});
    if (content.empty()) {
        sendTextResponse(clientSocket, 404, "Not Found", "text/plain",
                         "File not found.\n");
        return;
    }

    sendBinaryResponse(clientSocket, 200, "OK", getContentType(path), content);
}

void HttpServer::handlePost(const HttpRequest &request, int clientSocket) {
    if (request.path != "/plot") {
        sendTextResponse(clientSocket, 404, "Not Found", "text/plain",
                         "Unknown endpoint.\n");
        return;
    }

    const auto params = Utils::parseFormURLEncoded(request.body);
    const auto itX = params.find("xData");
    const auto itY = params.find("yData");
    const auto itType = params.find("plot");
    if (itX == params.end() || itY == params.end() || itType == params.end()) {
        sendTextResponse(clientSocket, 400, "Bad Request", "text/plain",
                         "Missing form fields.\n");
        return;
    }

    const std::vector<double> xs = Utils::parseNumberList(itX->second);
    const std::vector<double> ys = Utils::parseNumberList(itY->second);
    if (xs.empty() || ys.empty() || xs.size() != ys.size()) {
        sendTextResponse(clientSocket, 400, "Bad Request", "text/plain",
                         "X and Y arrays must have the same length and contain numbers.\n");
        return;
    }

    PlotRenderer renderer(800, 600);
    Image plot(800, 600);
    const std::string type = itType->second;
    if (type == "line") {
        plot = renderer.renderLinePlot(xs, ys);
    } else if (type == "scatter") {
        plot = renderer.renderScatterPlot(xs, ys);
    } else if (type == "bar") {
        plot = renderer.renderBarPlot(xs, ys);
    } else {
        sendTextResponse(clientSocket, 400, "Bad Request", "text/plain",
                         "Invalid plot type.\n");
        return;
    }

    const std::string body = plot.toBMP();
    sendBinaryResponse(clientSocket, 200, "OK", "image/bmp", body);
}

void HttpServer::sendTextResponse(int clientSocket, int statusCode, const std::string &statusText,
                                  const std::string &contentType, const std::string &body) const {
    sendBinaryResponse(clientSocket, statusCode, statusText, contentType, body);
}

void HttpServer::sendBinaryResponse(int clientSocket, int statusCode, const std::string &statusText,
                                    const std::string &contentType, const std::string &body) const {
    std::ostringstream response;
    response << getStatusLine(statusCode, statusText) << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Cache-Control: no-store\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n\r\n";

    const std::string headerStr = response.str();
    ::send(clientSocket, headerStr.data(), headerStr.size(), 0);
    if (!body.empty()) {
        ::send(clientSocket, body.data(), body.size(), 0);
    }
}

std::string HttpServer::getStatusLine(int statusCode, const std::string &statusText) {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << statusCode << ' ' << statusText;
    return ss.str();
}

std::string HttpServer::getContentType(const std::string &path) {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") {
        return "text/html; charset=utf-8";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") {
        return "text/css; charset=utf-8";
    }
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") {
        return "application/javascript";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".png") {
        return "image/png";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".ico") {
        return "image/x-icon";
    }
    return "text/plain; charset=utf-8";
}
