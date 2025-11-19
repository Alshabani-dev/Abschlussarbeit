#include "HttpServer.h"

#include "Utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>

HttpServer::HttpServer(int port)
    : port_(port), serverSocket_(-1), running_(false) {}

void HttpServer::stop() {
    running_ = false;
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
}

void HttpServer::run() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(serverSocket_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << "\n";
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    if (listen(serverSocket_, 16) < 0) {
        std::cerr << "Failed to listen on socket\n";
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    std::cout << "Server listening on port " << port_ << "\n";
    running_ = true;

    while (running_) {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);
        int clientSocket = accept(serverSocket_, reinterpret_cast<sockaddr *>(&clientAddress), &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection\n";
            }
            continue;
        }
        handleClient(clientSocket);
        close(clientSocket);
    }

    stop();
}

std::string HttpServer::readRequest(int clientSocket) {
    std::string data;
    char buffer[4096];
    size_t contentLength = 0;
    bool headerParsed = false;

    while (true) {
        ssize_t received = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break;
        }
        data.append(buffer, buffer + received);
        if (!headerParsed) {
            size_t headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headerParsed = true;
                std::string headerText = data.substr(0, headerEnd);
                std::istringstream stream(headerText);
                std::string line;
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    auto pos = line.find(':');
                    if (pos == std::string::npos) {
                        continue;
                    }
                    std::string key = line.substr(0, pos);
                    std::string value = Utils::trim(line.substr(pos + 1));
                    std::string lowerKey = key;
                    for (char &c : lowerKey) {
                        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    }
                    if (lowerKey == "content-length") {
                        contentLength = static_cast<size_t>(std::stoul(value));
                    }
                }
            }
        }
        if (headerParsed) {
            size_t headerEndPos = data.find("\r\n\r\n");
            if (headerEndPos != std::string::npos) {
                size_t totalSize = headerEndPos + 4 + contentLength;
                if (data.size() >= totalSize) {
                    break;
                }
            }
        }
    }
    return data;
}

bool HttpServer::parseRequest(const std::string &raw, HttpRequest &request) {
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false;
    }
    std::string headerText = raw.substr(0, headerEnd);
    request.body = raw.substr(headerEnd + 4);

    std::istringstream stream(headerText);
    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        return false;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }
    std::istringstream lineStream(requestLine);
    if (!(lineStream >> request.method >> request.path)) {
        return false;
    }

    std::string headerLine;
    while (std::getline(stream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        if (headerLine.empty()) {
            continue;
        }
        auto pos = headerLine.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = headerLine.substr(0, pos);
        std::string value = Utils::trim(headerLine.substr(pos + 1));
        for (char &c : key) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        request.headers[key] = value;
    }

    return true;
}

std::map<std::string, std::string> HttpServer::parseFormData(const std::string &body) {
    std::map<std::string, std::string> result;
    auto pairs = Utils::split(body, '&');
    for (const auto &pair : pairs) {
        auto pos = pair.find('=');
        std::string key = pos == std::string::npos ? pair : pair.substr(0, pos);
        std::string value = pos == std::string::npos ? "" : pair.substr(pos + 1);
        result[Utils::urlDecode(key)] = Utils::urlDecode(value);
    }
    return result;
}

void HttpServer::handleClient(int clientSocket) {
    std::string raw = readRequest(clientSocket);
    if (raw.empty()) {
        return;
    }
    HttpRequest request;
    if (!parseRequest(raw, request)) {
        sendResponse(clientSocket, "400 Bad Request", "text/plain", "Malformed HTTP request");
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

std::string HttpServer::getContentType(const std::string &path) const {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") {
        return "text/html";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") {
        return "text/css";
    }
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") {
        return "application/javascript";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".bmp") {
        return "image/bmp";
    }
    return "text/plain";
}

void HttpServer::handleGetRequest(int clientSocket, const HttpRequest &request) {
    std::string path = request.path;
    if (path == "/") {
        path = "/index.html";
    }
    if (path.size() > 1 && path[0] == '/') {
        path = path.substr(1);
    }
    if (path.find("..") != std::string::npos) {
        sendResponse(clientSocket, "403 Forbidden", "text/plain", "Invalid path");
        return;
    }

    std::string fullPath = "public/" + path;
    std::string content = Utils::readFile(fullPath);
    if (content.empty()) {
        content = Utils::readFile("../" + fullPath);
    }
    if (content.empty()) {
        sendResponse(clientSocket, "404 Not Found", "text/plain", "File not found");
        return;
    }

    sendResponse(clientSocket, "200 OK", getContentType(fullPath), content);
}

void HttpServer::handlePostRequest(int clientSocket, const HttpRequest &request) {
    if (request.path != "/plot") {
        sendResponse(clientSocket, "404 Not Found", "text/plain", "Unknown endpoint");
        return;
    }

    auto form = parseFormData(request.body);
    auto plotIt = form.find("plot");
    auto xIt = form.find("xData");
    auto yIt = form.find("yData");
    if (plotIt == form.end() || xIt == form.end() || yIt == form.end()) {
        sendResponse(clientSocket, "400 Bad Request", "text/plain", "Missing form fields");
        return;
    }

    auto xValues = Utils::parseNumberList(xIt->second);
    auto yValues = Utils::parseNumberList(yIt->second);
    if (xValues.empty() || yValues.empty() || xValues.size() != yValues.size()) {
        sendResponse(clientSocket, "400 Bad Request", "text/plain", "Invalid numeric data");
        return;
    }

    std::string plotType = plotIt->second;
    for (char &c : plotType) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    PlotType type;
    if (plotType == "line") {
        type = PlotType::Line;
    } else if (plotType == "scatter") {
        type = PlotType::Scatter;
    } else if (plotType == "bar") {
        type = PlotType::Bar;
    } else {
        sendResponse(clientSocket, "400 Bad Request", "text/plain", "Invalid plot type");
        return;
    }

    Image image = renderer_.renderPlot(xValues, yValues, type);
    std::string bmp = image.toBMP();
    sendResponse(clientSocket, "200 OK", "image/bmp", bmp, {{"Content-Disposition", "inline"}});
}

void HttpServer::sendResponse(int clientSocket,
                              const std::string &status,
                              const std::string &contentType,
                              const std::string &body,
                              const std::vector<std::pair<std::string, std::string>> &extraHeaders) {
    auto sendAll = [&](const char *buffer, size_t length) {
        size_t total = 0;
        while (total < length) {
            ssize_t sent = send(clientSocket, buffer + total, length - total, 0);
            if (sent <= 0) {
                break;
            }
            total += static_cast<size_t>(sent);
        }
    };

    std::ostringstream header;
    header << "HTTP/1.1 " << status << "\r\n";
    header << "Content-Type: " << contentType << "\r\n";
    header << "Content-Length: " << body.size() << "\r\n";
    header << "Connection: close\r\n";
    header << "Cache-Control: no-store\r\n";
    for (const auto &entry : extraHeaders) {
        header << entry.first << ": " << entry.second << "\r\n";
    }
    header << "\r\n";

    std::string data = header.str();
    sendAll(data.c_str(), data.size());
    if (!body.empty()) {
        sendAll(body.data(), body.size());
    }
}
