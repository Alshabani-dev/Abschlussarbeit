#include "HttpServer.h"

#include "PlotRenderer.h"
#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <arpa/inet.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

HttpServer::HttpServer(int port) : port_(port) {}

void HttpServer::run() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::perror("bind");
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 10) < 0) {
        std::perror("listen");
        close(serverSocket);
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            std::perror("accept");
            continue;
        }
        handleClient(clientSocket);
        close(clientSocket);
    }

    close(serverSocket);
}

void HttpServer::handleClient(int clientSocket) const {
    std::string raw = readRequest(clientSocket);
    if (raw.empty()) {
        return;
    }

    HttpRequest request;
    if (!parseRequest(raw, request)) {
        sendResponse(clientSocket, "400 Bad Request", "text/plain", "Invalid request");
        return;
    }

    if (request.method == "GET") {
        handleGetRequest(clientSocket, request);
    } else if (request.method == "POST") {
        handlePostRequest(clientSocket, request);
    } else {
        sendResponse(clientSocket, "405 Method Not Allowed", "text/plain", "Unsupported method");
    }
}

std::string HttpServer::readRequest(int clientSocket) const {
    std::string data;
    char buffer[4096];
    ssize_t received;
    size_t contentLength = 0;
    bool hasLength = false;
    size_t headerEnd = std::string::npos;

    while ((received = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        data.append(buffer, received);
        if (headerEnd == std::string::npos) {
            headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                std::string headerSection = data.substr(0, headerEnd);
                std::istringstream stream(headerSection);
                std::string line;
                if (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                }
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    size_t colon = line.find(':');
                    if (colon == std::string::npos) continue;
                    std::string name = trim(line.substr(0, colon));
                    std::string value = trim(line.substr(colon + 1));
                    std::transform(name.begin(), name.end(), name.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    if (name == "content-length") {
                        contentLength = static_cast<size_t>(std::stoul(value));
                        hasLength = true;
                    }
                }
            }
        }

        if (headerEnd != std::string::npos) {
            size_t expectedSize = headerEnd + 4;
            if (hasLength) {
                expectedSize += contentLength;
            }
            if (data.size() >= expectedSize) {
                break;
            }
        }

        if (data.size() > 1024 * 1024) {
            break;
        }
    }

    return data;
}

bool HttpServer::parseRequest(const std::string& raw, HttpRequest& request) const {
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false;
    }

    std::string headerPart = raw.substr(0, headerEnd);
    request.body = raw.substr(headerEnd + 4);

    std::istringstream stream(headerPart);
    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        return false;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    requestLineStream >> request.method >> request.path;
    std::string version;
    requestLineStream >> version;

    if (request.method.empty() || request.path.empty()) {
        return false;
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string name = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        request.headers[name] = value;
    }

    return true;
}

void HttpServer::handleGetRequest(int clientSocket, const HttpRequest& request) const {
    if (request.path.find("..") != std::string::npos) {
        sendResponse(clientSocket, "403 Forbidden", "text/plain", "Invalid path");
        return;
    }

    std::string content;
    std::string path = request.path;
    if (path == "/") {
        content = readFile("public/index.html");
        if (content.empty()) {
            content = readFile("../public/index.html");
        }
        if (content.empty()) {
            sendResponse(clientSocket, "404 Not Found", "text/plain", "File not found");
            return;
        }
        sendResponse(clientSocket, "200 OK", "text/html; charset=UTF-8", content);
        return;
    }

    std::string localPath = "public" + path;
    content = readFile(localPath);
    if (content.empty()) {
        content = readFile("../" + localPath);
    }
    if (content.empty()) {
        sendResponse(clientSocket, "404 Not Found", "text/plain", "File not found");
        return;
    }

    sendResponse(clientSocket, "200 OK", getContentType(path), content);
}

void HttpServer::handlePostRequest(int clientSocket, const HttpRequest& request) const {
    if (request.path != "/plot") {
        sendResponse(clientSocket, "404 Not Found", "text/plain", "Unknown endpoint");
        return;
    }

    auto fields = parseFormData(request.body);
    auto xIt = fields.find("xValues");
    auto yIt = fields.find("yValues");
    auto plotIt = fields.find("plot");

    if (xIt == fields.end() || yIt == fields.end() || plotIt == fields.end()) {
        sendResponse(clientSocket, "400 Bad Request", "text/plain", "Missing form fields");
        return;
    }

    std::vector<double> xs = parseNumberList(xIt->second);
    std::vector<double> ys = parseNumberList(yIt->second);

    if (xs.empty() || ys.empty()) {
        sendResponse(clientSocket, "400 Bad Request", "text/plain", "Invalid numeric data");
        return;
    }

    PlotRenderer renderer(800, 600);
    Image image(800, 600);

    if (plotIt->second == "line") {
        image = renderer.renderLinePlot(xs, ys);
    } else if (plotIt->second == "scatter") {
        image = renderer.renderScatterPlot(xs, ys);
    } else if (plotIt->second == "bar") {
        image = renderer.renderBarPlot(xs, ys);
    } else {
        sendResponse(clientSocket, "400 Bad Request", "text/plain", "Invalid plot type");
        return;
    }

    std::string body = image.toBMP();

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: image/bmp\r\n"
             << "Cache-Control: no-store\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n";
    std::string header = response.str();
    send(clientSocket, header.data(), header.size(), 0);
    send(clientSocket, body.data(), body.size(), 0);
}

void HttpServer::sendResponse(int clientSocket, const std::string& status,
                              const std::string& contentType, const std::string& body) const {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "Connection: close\r\n\r\n"
             << body;
    std::string data = response.str();
    send(clientSocket, data.data(), data.size(), 0);
}

std::string HttpServer::getContentType(const std::string& path) const {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") {
        return "text/html; charset=UTF-8";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") {
        return "text/css; charset=UTF-8";
    }
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") {
        return "application/javascript";
    }
    return "application/octet-stream";
}
