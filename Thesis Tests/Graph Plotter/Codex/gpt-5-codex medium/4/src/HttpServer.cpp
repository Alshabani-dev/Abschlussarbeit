#include "HttpServer.h"

#include "PlotRenderer.h"
#include "Utils.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <vector>
#include <unistd.h>

namespace {
constexpr int RENDER_WIDTH = 900;
constexpr int RENDER_HEIGHT = 600;
}

HttpServer::HttpServer(int port, std::string publicDir)
    : port_(port), publicDir_(std::move(publicDir)) {}

void HttpServer::start() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int enable = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd, 16) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Server listening on port " << port_ << std::endl;
    runLoop(serverFd);
    close(serverFd);
}

void HttpServer::runLoop(int serverFd) {
    while (true) {
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr *>(&client), &len);
        if (clientFd < 0) {
            continue;
        }
        handleClient(clientFd);
        close(clientFd);
    }
}

bool HttpServer::readRawRequest(int clientFd, std::string &raw) const {
    raw.clear();
    char buffer[4096];
    ssize_t bytes;
    size_t headerEnd = std::string::npos;
    size_t bodyLength = 0;
    bool lengthParsed = false;

    while ((bytes = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
        raw.append(buffer, bytes);
        if (headerEnd == std::string::npos) {
            headerEnd = raw.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                std::string headerSection = raw.substr(0, headerEnd);
                std::istringstream headerStream(headerSection);
                std::string line;
                if (std::getline(headerStream, line)) {
                    // ignore request line
                }
                while (std::getline(headerStream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    auto colon = line.find(':');
                    if (colon == std::string::npos) {
                        continue;
                    }
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    key = Utils::trim(key);
                    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
                        return static_cast<char>(std::tolower(c));
                    });
                    if (key == "content-length") {
                        value = Utils::trim(value);
                        bodyLength = static_cast<size_t>(std::stoul(value));
                        lengthParsed = true;
                        break;
                    }
                }
                if (!lengthParsed) {
                    bodyLength = 0;
                }
            }
        }

        if (headerEnd != std::string::npos) {
            size_t totalNeeded = headerEnd + 4 + bodyLength;
            if (raw.size() >= totalNeeded) {
                return true;
            }
        }
    }

    return false;
}

HttpServer::HttpRequest HttpServer::parseRequest(const std::string &raw) const {
    HttpRequest request;
    size_t headerEnd = raw.find("\r\n\r\n");
    std::string headerPart = (headerEnd == std::string::npos) ? raw : raw.substr(0, headerEnd);
    request.body = (headerEnd == std::string::npos) ? std::string() : raw.substr(headerEnd + 4);

    std::istringstream stream(headerPart);
    std::string requestLine;
    if (std::getline(stream, requestLine)) {
        if (!requestLine.empty() && requestLine.back() == '\r') {
            requestLine.pop_back();
        }
        std::istringstream lineStream(requestLine);
        lineStream >> request.method >> request.path;
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = Utils::trim(line.substr(0, colon));
        std::string value = Utils::trim(line.substr(colon + 1));
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        request.headers[key] = value;
    }

    return request;
}

void HttpServer::handleClient(int clientFd) {
    std::string raw;
    if (!readRawRequest(clientFd, raw)) {
        sendError(clientFd, 400, "Bad Request");
        return;
    }

    auto request = parseRequest(raw);
    if (request.method == "GET") {
        handleGetRequest(clientFd, request);
    } else if (request.method == "POST") {
        handlePostRequest(clientFd, request);
    } else {
        sendError(clientFd, 405, "Method Not Allowed");
    }
}

void HttpServer::handleGetRequest(int clientFd, const HttpRequest &request) {
    std::string path = request.path;
    if (path == "/") {
        path = "/index.html";
    }

    if (path.find("..") != std::string::npos) {
        sendError(clientFd, 403, "Forbidden");
        return;
    }

    std::string fileData = readFile(path);
    if (fileData.empty()) {
        sendError(clientFd, 404, "File Not Found");
        return;
    }

    auto contentType = getContentType(path);
    sendResponse(clientFd, "200 OK", contentType, fileData, true, "Cache-Control: no-store\r\n");
}

void HttpServer::handlePostRequest(int clientFd, const HttpRequest &request) {
    if (request.path != "/plot") {
        sendError(clientFd, 404, "Not Found");
        return;
    }

    std::unordered_map<std::string, std::string> form;
    if (!Utils::parseUrlEncodedBody(request.body, form)) {
        sendError(clientFd, 400, "Invalid form data");
        return;
    }

    auto xIt = form.find("xData");
    auto yIt = form.find("yData");
    auto plotIt = form.find("plot");
    if (xIt == form.end() || yIt == form.end() || plotIt == form.end()) {
        sendError(clientFd, 400, "Missing form fields");
        return;
    }

    try {
        auto xValues = Utils::parseNumberList(xIt->second);
        auto yValues = Utils::parseNumberList(yIt->second);
        if (xValues.size() != yValues.size()) {
            throw std::runtime_error("Mismatched X/Y lengths");
        }
        std::string plotType = plotIt->second;
        PlotRenderer renderer(RENDER_WIDTH, RENDER_HEIGHT);
        Image image = [&]() {
            if (plotType == "line") {
                return renderer.renderLinePlot(xValues, yValues);
            }
            if (plotType == "scatter") {
                return renderer.renderScatterPlot(xValues, yValues);
            }
            if (plotType == "bar") {
                return renderer.renderBarPlot(xValues, yValues);
            }
            throw std::runtime_error("Invalid plot type");
        }();

        std::string body = image.toBMP();
        sendResponse(clientFd, "200 OK", "image/bmp", body, true,
                     "Cache-Control: no-store\r\n");
    } catch (const std::exception &ex) {
        sendError(clientFd, 400, ex.what());
    }
}

std::string HttpServer::readFile(const std::string &path) const {
    std::string sanitized = path;
    if (!sanitized.empty() && sanitized.front() == '/') {
        sanitized.erase(0, 1);
    }

    std::vector<std::string> candidates = {
        publicDir_ + "/" + sanitized,
        "../" + publicDir_ + "/" + sanitized
    };

    for (const auto &candidate : candidates) {
        std::ifstream file(candidate, std::ios::binary);
        if (!file.good()) {
            continue;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    return {};
}

std::string HttpServer::getContentType(const std::string &path) const {
    auto hasExtension = [&](const std::string &ext) {
        return path.size() >= ext.size() &&
               path.compare(path.size() - ext.size(), ext.size(), ext) == 0;
    };
    if (hasExtension(".html")) {
        return "text/html; charset=utf-8";
    }
    if (hasExtension(".css")) {
        return "text/css; charset=utf-8";
    }
    if (hasExtension(".js")) {
        return "application/javascript";
    }
    return "application/octet-stream";
}

void HttpServer::sendResponse(int clientFd, const std::string &status,
                              const std::string &contentType, const std::string &body,
                              bool /*binary*/, const std::string &extraHeaders) const {
    std::ostringstream headers;
    headers << "HTTP/1.1 " << status << "\r\n";
    headers << "Content-Type: " << contentType << "\r\n";
    headers << "Content-Length: " << body.size() << "\r\n";
    if (!extraHeaders.empty()) {
        headers << extraHeaders;
    }
    headers << "Connection: close\r\n\r\n";

    std::string response = headers.str();
    send(clientFd, response.data(), response.size(), 0);
    if (!body.empty()) {
        send(clientFd, body.data(), body.size(), 0);
    }
}

void HttpServer::sendError(int clientFd, int statusCode, const std::string &message) const {
    std::ostringstream body;
    body << "<html><body><h1>" << statusCode << " - " << message << "</h1></body></html>";
    std::ostringstream status;
    status << statusCode << " " << (statusCode == 404 ? "Not Found" : "Error");
    sendResponse(clientFd, status.str(), "text/html; charset=utf-8", body.str());
}
