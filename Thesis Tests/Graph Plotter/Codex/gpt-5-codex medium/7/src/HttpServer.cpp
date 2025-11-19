#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "PlotRenderer.h"
#include "Utils.h"

namespace {
constexpr int kDefaultWidth = 800;
constexpr int kDefaultHeight = 600;
}

HttpServer::HttpServer(int port) : port_(port), serverFd_(-1), running_(false) {}

HttpServer::~HttpServer() { stop(); }

void HttpServer::start() {
    serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(serverFd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket to port " + std::to_string(port_));
    }

    if (listen(serverFd_, 16) < 0) {
        throw std::runtime_error("Failed to listen on port");
    }

    running_ = true;
    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in client{};
        socklen_t clientLen = sizeof(client);
        int clientFd = accept(serverFd_, reinterpret_cast<sockaddr *>(&client), &clientLen);
        if (clientFd < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection: " << std::strerror(errno) << std::endl;
            }
            continue;
        }
        handleClient(clientFd);
        close(clientFd);
    }
}

void HttpServer::stop() {
    running_ = false;
    if (serverFd_ >= 0) {
        shutdown(serverFd_, SHUT_RDWR);
        close(serverFd_);
        serverFd_ = -1;
    }
}

void HttpServer::handleClient(int clientFd) {
    std::string rawRequest = readRequest(clientFd);
    if (rawRequest.empty()) {
        return;
    }

    HttpRequest request = parseRequest(rawRequest);
    if (request.method.empty()) {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Malformed request\n");
        return;
    }

    if (request.method == "GET") {
        handleGet(request, clientFd);
    } else if (request.method == "POST") {
        handlePost(request, clientFd);
    } else {
        sendResponse(clientFd, "405 Method Not Allowed", "text/plain", "Method not allowed\n");
    }
}

std::string HttpServer::readRequest(int clientFd) {
    std::string data;
    data.reserve(4096);

    char buffer[4096];
    ssize_t bytesRead = 0;
    size_t expectedBody = 0;
    bool headersParsed = false;
    size_t totalExpected = 0;

    while ((bytesRead = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
        data.append(buffer, static_cast<size_t>(bytesRead));
        if (!headersParsed) {
            size_t headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headersParsed = true;
                std::string headerSection = data.substr(0, headerEnd);
                std::istringstream stream(headerSection);
                std::string line;
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    auto colon = line.find(':');
                    if (colon == std::string::npos) {
                        continue;
                    }
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    key.erase(std::remove_if(key.begin(), key.end(), [](unsigned char ch) { return std::isspace(ch); }),
                              key.end());
                    value = utils::trim(value);
                    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
                    if (key == "content-length") {
                        try {
                            expectedBody = static_cast<size_t>(std::stoul(value));
                        } catch (...) {
                            expectedBody = 0;
                        }
                        break;
                    }
                }
                totalExpected = headerEnd + 4 + expectedBody;
                if (expectedBody == 0) {
                    break;
                }
            }
        }

        if (headersParsed && expectedBody > 0 && data.size() >= totalExpected) {
            break;
        }
    }

    return data;
}

HttpServer::HttpRequest HttpServer::parseRequest(const std::string &rawRequest) {
    HttpRequest request;
    size_t lineEnd = rawRequest.find("\r\n");
    if (lineEnd == std::string::npos) {
        return request;
    }

    std::string requestLine = rawRequest.substr(0, lineEnd);
    std::istringstream lineStream(requestLine);
    lineStream >> request.method >> request.path;

    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return request;
    }

    size_t headerStart = lineEnd + 2;
    size_t cursor = headerStart;
    while (cursor < headerEnd) {
        size_t next = rawRequest.find("\r\n", cursor);
        if (next == std::string::npos || next > headerEnd) {
            break;
        }
        std::string headerLine = rawRequest.substr(cursor, next - cursor);
        auto colon = headerLine.find(':');
        if (colon != std::string::npos) {
            std::string key = headerLine.substr(0, colon);
            std::string value = headerLine.substr(colon + 1);
            value = utils::trim(value);
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
            request.headers[key] = value;
        }
        cursor = next + 2;
    }

    request.body = rawRequest.substr(headerEnd + 4);
    return request;
}

std::unordered_map<std::string, std::string> HttpServer::parseFormData(const std::string &body) {
    std::unordered_map<std::string, std::string> values;
    size_t start = 0;
    while (start < body.size()) {
        size_t amp = body.find('&', start);
        if (amp == std::string::npos) {
            amp = body.size();
        }
        std::string pair = body.substr(start, amp - start);
        size_t equals = pair.find('=');
        if (equals != std::string::npos) {
            std::string key = utils::urlDecode(pair.substr(0, equals));
            std::string value = utils::urlDecode(pair.substr(equals + 1));
            values[key] = value;
        }
        start = amp + 1;
    }
    return values;
}

void HttpServer::handleGet(const HttpRequest &request, int clientFd) {
    std::string path = request.path;
    if (path == "/") {
        path = "/index.html";
    }

    if (path.find("..") != std::string::npos) {
        sendResponse(clientFd, "403 Forbidden", "text/plain", "Forbidden\n");
        return;
    }

    const std::string primary = "public" + path;
    bool ok = false;
    std::string content = utils::readFile(primary, &ok);
    std::string usedPath = primary;

    if (!ok) {
        const std::string fallback = "../" + primary;
        content = utils::readFile(fallback, &ok);
        if (ok) {
            usedPath = fallback;
        }
    }

    if (!ok) {
        sendResponse(clientFd, "404 Not Found", "text/plain", "Not found\n");
        return;
    }

    std::string headers = "Cache-Control: no-cache\r\n";
    sendResponse(clientFd, "200 OK", utils::getContentType(usedPath), content, headers);
}

void HttpServer::handlePost(const HttpRequest &request, int clientFd) {
    if (request.path != "/plot") {
        sendResponse(clientFd, "404 Not Found", "text/plain", "Unknown endpoint\n");
        return;
    }

    auto form = parseFormData(request.body);
    auto itPlot = form.find("plot");
    auto itX = form.find("xData");
    auto itY = form.find("yData");

    if (itPlot == form.end() || itX == form.end() || itY == form.end()) {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Missing form fields\n");
        return;
    }

    auto xValues = utils::parseDoubles(itX->second);
    auto yValues = utils::parseDoubles(itY->second);

    if (xValues.size() != yValues.size() || xValues.empty()) {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Coordinate arrays must be equal length\n");
        return;
    }

    PlotRenderer renderer(kDefaultWidth, kDefaultHeight);
    std::string plotType = itPlot->second;
    std::transform(plotType.begin(), plotType.end(), plotType.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (plotType != "line" && plotType != "scatter" && plotType != "bar") {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Invalid plot type\n");
        return;
    }

    Image plotImage = [&]() -> Image {
        if (plotType == "line") {
            return renderer.renderLinePlot(xValues, yValues);
        }
        if (plotType == "scatter") {
            return renderer.renderScatterPlot(xValues, yValues);
        }
        return renderer.renderBarPlot(xValues, yValues);
    }();

    std::string body = plotImage.toBMP();
    std::string headers = "Cache-Control: no-store\r\n";
    sendResponse(clientFd, "200 OK", "image/bmp", body, headers);
}

void HttpServer::sendResponse(int clientFd, const std::string &status, const std::string &contentType,
                              const std::string &body, const std::string &extraHeaders) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    if (!extraHeaders.empty()) {
        response << extraHeaders;
    }
    response << "\r\n";

    std::string headerData = response.str();
    send(clientFd, headerData.data(), headerData.size(), 0);
    if (!body.empty()) {
        size_t totalSent = 0;
        while (totalSent < body.size()) {
            ssize_t sent = send(clientFd, body.data() + totalSent, body.size() - totalSent, 0);
            if (sent <= 0) {
                break;
            }
            totalSent += static_cast<size_t>(sent);
        }
    }
}
