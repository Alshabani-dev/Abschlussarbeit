#include "HttpServer.h"

#include "Utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {
std::string readStaticFile(const std::string& relativePath) {
    std::string content = Utils::readFile("public/" + relativePath);
    if (!content.empty()) {
        return content;
    }
    return Utils::readFile("../public/" + relativePath);
}
}

HttpServer::HttpServer(int port)
    : port_(port), serverFd_(-1), renderer_(800, 600) {}

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

    if (bind(serverFd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd_, 10) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Server running on port " << port_ << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            continue;
        }
        handleClient(clientFd);
        close(clientFd);
    }
}

std::string HttpServer::readRequest(int clientFd) {
    std::string request;
    char buffer[4096];
    ssize_t bytesRead;
    size_t contentLength = 0;
    bool headersParsed = false;

    while ((bytesRead = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, buffer + bytesRead);
        if (!headersParsed) {
            size_t headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headersParsed = true;
                const std::string headers = request.substr(0, headerEnd);
                const std::string key = "Content-Length:";
                const auto pos = headers.find(key);
                if (pos != std::string::npos) {
                    size_t lineEnd = headers.find('\n', pos);
                    const std::string lengthStr = Utils::trim(headers.substr(pos + key.size(), lineEnd - pos - key.size()));
                    contentLength = static_cast<size_t>(std::stoi(lengthStr));
                }
                const size_t totalLength = headerEnd + 4 + contentLength;
                if (contentLength == 0 || request.size() >= totalLength) {
                    break;
                }
            }
        } else if (contentLength > 0) {
            size_t headerEnd = request.find("\r\n\r\n");
            const size_t totalLength = headerEnd + 4 + contentLength;
            if (request.size() >= totalLength) {
                break;
            }
        } else {
            break;
        }
        if (bytesRead < static_cast<ssize_t>(sizeof(buffer))) {
            break;
        }
    }

    return request;
}

void HttpServer::handleClient(int clientFd) {
    const std::string request = readRequest(clientFd);
    if (request.empty()) {
        return;
    }

    const size_t lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos) {
        return;
    }
    const std::string requestLine = request.substr(0, lineEnd);
    std::istringstream lineStream(requestLine);
    std::string method;
    std::string path;
    std::string version;
    lineStream >> method >> path >> version;

    std::string body;
    const size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd != std::string::npos && headerEnd + 4 < request.size()) {
        body = request.substr(headerEnd + 4);
    }

    if (method == "GET") {
        handleGetRequest(clientFd, path);
    } else if (method == "POST" && path == "/plot") {
        handlePostRequest(clientFd, body);
    } else {
        sendResponse(clientFd, "404 Not Found", "text/plain", "Not Found");
    }
}

void HttpServer::handleGetRequest(int clientFd, const std::string& path) {
    std::string relativePath = path;
    if (relativePath == "/") {
        relativePath = "/index.html";
    }
    if (!relativePath.empty() && relativePath.front() == '/') {
        relativePath.erase(relativePath.begin());
    }

    const std::string content = readStaticFile(relativePath);
    if (content.empty()) {
        sendResponse(clientFd, "404 Not Found", "text/plain", "File not found");
        return;
    }

    sendResponse(clientFd, "200 OK", getContentType(relativePath), content);
}

void HttpServer::handlePostRequest(int clientFd, const std::string& body) {
    const auto form = Utils::parseFormUrlEncoded(body);
    const auto xIt = form.find("xValues");
    const auto yIt = form.find("yValues");
    const auto plotIt = form.find("plot");

    if (xIt == form.end() || yIt == form.end() || plotIt == form.end()) {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Missing form fields");
        return;
    }

    const auto xValues = Utils::parseDoubleList(xIt->second);
    const auto yValues = Utils::parseDoubleList(yIt->second);
    if (xValues.empty() || yValues.empty() || xValues.size() != yValues.size()) {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Invalid coordinate data");
        return;
    }

    const std::string plotType = plotIt->second;
    Image plotImage = renderer_.renderLinePlot(xValues, yValues);
    if (plotType == "scatter") {
        plotImage = renderer_.renderScatterPlot(xValues, yValues);
    } else if (plotType == "bar") {
        plotImage = renderer_.renderBarPlot(xValues, yValues);
    } else if (plotType != "line") {
        sendResponse(clientFd, "400 Bad Request", "text/plain", "Invalid plot type");
        return;
    }

    const std::string bmpData = plotImage.toBMP();
    sendResponse(clientFd, "200 OK", "image/bmp", bmpData);
}

void HttpServer::sendResponse(int clientFd, const std::string& status,
                              const std::string& contentType, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Cache-Control: no-store\r\n";
    response << "Connection: close\r\n\r\n";
    std::string header = response.str();
    send(clientFd, header.data(), header.size(), 0);
    if (!body.empty()) {
        send(clientFd, body.data(), body.size(), 0);
    }
}

std::string HttpServer::getContentType(const std::string& path) const {
    if (path.find(".css") != std::string::npos) {
        return "text/css";
    }
    if (path.find(".js") != std::string::npos) {
        return "application/javascript";
    }
    if (path.find(".bmp") != std::string::npos) {
        return "image/bmp";
    }
    return "text/html";
}
