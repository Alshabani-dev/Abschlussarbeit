#include "HttpServer.h"

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "Utils.h"

HttpServer::HttpServer(int port, std::string publicDir)
    : port_(port), publicDir_(std::move(publicDir)), running_(false), serverFd_(-1), renderer_(800, 600) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int enable = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd_, 8) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    running_ = true;
    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            if (running_) {
                continue;
            }
            break;
        }
        handleClient(clientFd);
        close(clientFd);
    }
}

void HttpServer::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    if (serverFd_ >= 0) {
        shutdown(serverFd_, SHUT_RDWR);
        close(serverFd_);
        serverFd_ = -1;
    }
}

std::string HttpServer::readRequest(int clientFd) {
    std::string data;
    char buffer[4096];
    ssize_t bytes = 0;
    size_t contentLength = 0;
    bool headersParsed = false;
    size_t headerEnd = std::string::npos;

    while ((bytes = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
        data.append(buffer, bytes);
        if (!headersParsed) {
            headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headersParsed = true;
                std::istringstream headerStream(data.substr(0, headerEnd));
                std::string line;
                while (std::getline(headerStream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    if (line.rfind("Content-Length:", 0) == 0) {
                        contentLength = static_cast<size_t>(std::stoul(utils::trim(line.substr(15))));
                    }
                }
            }
        }
        if (headersParsed) {
            const size_t totalNeeded = headerEnd + 4 + contentLength;
            if (data.size() >= totalNeeded) {
                break;
            }
        }
        if (bytes < static_cast<ssize_t>(sizeof(buffer))) {
            break;
        }
    }
    return data;
}

void HttpServer::handleClient(int clientFd) {
    std::string request = readRequest(clientFd);
    if (request.empty()) {
        return;
    }

    auto lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos) {
        return;
    }
    std::string requestLine = request.substr(0, lineEnd);
    std::istringstream requestStream(requestLine);
    std::string method;
    std::string path;
    std::string version;
    requestStream >> method >> path >> version;

    size_t headerEnd = request.find("\r\n\r\n");
    std::string body;
    if (headerEnd != std::string::npos && headerEnd + 4 <= request.size()) {
        body = request.substr(headerEnd + 4);
    }

    if (method == "GET") {
        handleGet(clientFd, path);
    } else if (method == "POST" && path == "/plot") {
        handlePostPlot(clientFd, body);
    } else {
        sendResponse(clientFd, "404 Not Found", "text/plain", "Not Found");
    }
}

PlotRenderer::PlotType HttpServer::parsePlotType(const std::string& value) const {
    return PlotRenderer::plotTypeFromString(value);
}

std::string HttpServer::locatePublicFile(const std::string& requestedPath) const {
    const std::string sanitized = (requestedPath.front() == '/') ? requestedPath.substr(1) : requestedPath;
    const std::vector<std::string> prefixes = {
        publicDir_,
        "../" + publicDir_
    };

    for (const std::string& prefix : prefixes) {
        std::string base = prefix;
        if (!base.empty() && base.back() != '/') {
            base.push_back('/');
        }
        std::string candidate = base + sanitized;
        std::ifstream file(candidate, std::ios::binary);
        if (file.good()) {
            return candidate;
        }
    }
    return {};
}

void HttpServer::handleGet(int clientFd, const std::string& rawPath) {
    std::string path = rawPath;
    if (path == "/") {
        path = "/index.html";
    }
    if (path.find("..") != std::string::npos) {
        sendResponse(clientFd, "403 Forbidden", "text/plain", "Forbidden");
        return;
    }

    std::string filePath = locatePublicFile(path);
    if (filePath.empty()) {
        sendResponse(clientFd, "404 Not Found", "text/plain", "File Not Found");
        return;
    }

    std::string content = utils::readFile(filePath);
    sendResponse(clientFd, "200 OK", getContentType(path), content);
}

void HttpServer::handlePostPlot(int clientFd, const std::string& body) {
    auto form = utils::parseFormUrlEncoded(body);
    const auto xIt = form.find("x");
    const auto yIt = form.find("y");
    const auto plotIt = form.find("plot");

    if (xIt == form.end() || yIt == form.end() || plotIt == form.end()) {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Missing form fields");
        return;
    }

    std::vector<double> xs;
    std::vector<double> ys;
    if (!utils::parseNumberList(xIt->second, xs) || !utils::parseNumberList(yIt->second, ys)) {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Invalid numeric data");
        return;
    }

    if (xs.size() != ys.size()) {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "X and Y arrays must have equal length");
        return;
    }

    PlotRenderer::PlotType plotType = parsePlotType(plotIt->second);
    Image image = renderer_.renderPlot(xs, ys, plotType);
    std::string bmp = image.toBMP();
    sendResponse(clientFd, "200 OK", "image/bmp", bmp);
}

std::string HttpServer::getContentType(const std::string& path) const {
    if (path.find(".css") != std::string::npos) {
        return "text/css";
    }
    if (path.find(".js") != std::string::npos) {
        return "application/javascript";
    }
    if (path.find(".png") != std::string::npos) {
        return "image/png";
    }
    return "text/html";
}

void HttpServer::sendResponse(int clientFd, const std::string& status,
                              const std::string& contentType,
                              const std::string& body) {
    std::ostringstream headers;
    headers << "HTTP/1.1 " << status << "\r\n";
    headers << "Content-Type: " << contentType << "\r\n";
    headers << "Cache-Control: no-store\r\n";
    headers << "Content-Length: " << body.size() << "\r\n";
    headers << "Connection: close\r\n\r\n";
    std::string headerStr = headers.str();
    send(clientFd, headerStr.data(), headerStr.size(), 0);
    if (!body.empty()) {
        send(clientFd, body.data(), body.size(), 0);
    }
}
