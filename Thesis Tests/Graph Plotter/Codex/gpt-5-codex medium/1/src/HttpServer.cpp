#include "HttpServer.h"

#include "Utils.h"

#include <arpa/inet.h>
#include <cctype>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
bool hasSuffix(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string detectContentType(const std::string& path) {
    if (hasSuffix(path, ".html")) return "text/html; charset=utf-8";
    if (hasSuffix(path, ".css")) return "text/css; charset=utf-8";
    if (hasSuffix(path, ".js")) return "application/javascript";
    if (hasSuffix(path, ".png")) return "image/png";
    if (hasSuffix(path, ".jpg") || hasSuffix(path, ".jpeg")) return "image/jpeg";
    if (hasSuffix(path, ".bmp")) return "image/bmp";
    return "application/octet-stream";
}

PlotType parsePlotType(const std::string& type) {
    if (type == "scatter") return PlotType::Scatter;
    if (type == "bar") return PlotType::Bar;
    return PlotType::Line;
}
}

HttpServer::HttpServer(int port)
    : port_(port), serverFd_(-1), running_(false) {}

HttpServer::~HttpServer() {
    if (serverFd_ >= 0) {
        close(serverFd_);
    }
}

void HttpServer::run() {
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind port");
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
    running_ = false;
    if (serverFd_ >= 0) {
        shutdown(serverFd_, SHUT_RDWR);
        close(serverFd_);
        serverFd_ = -1;
    }
}

void HttpServer::handleClient(int clientFd) {
    try {
        Request request = readRequest(clientFd);
        if (request.method == "GET") {
            handleGet(clientFd, request);
        } else if (request.method == "POST") {
            handlePost(clientFd, request);
        } else {
            sendResponse(clientFd, "HTTP/1.1 405 Method Not Allowed", "text/plain", "Method Not Allowed");
        }
    } catch (const std::exception& ex) {
        std::cerr << "Request handling failed: " << ex.what() << std::endl;
    }
}

HttpServer::Request HttpServer::readRequest(int clientFd) {
    Request request;
    std::string buffer;
    buffer.reserve(4096);

    char temp[2048];
    ssize_t bytesRead;
    size_t headerEnd = std::string::npos;
    size_t contentLength = 0;

    while ((bytesRead = recv(clientFd, temp, sizeof(temp), 0)) > 0) {
        buffer.append(temp, bytesRead);
        if (headerEnd == std::string::npos) {
            headerEnd = buffer.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                std::string headerSection = buffer.substr(0, headerEnd);
                std::istringstream headerStream(headerSection);
                std::string requestLine;
                std::getline(headerStream, requestLine);
                if (!requestLine.empty() && requestLine.back() == '\r') {
                    requestLine.pop_back();
                }
                std::istringstream lineStream(requestLine);
                lineStream >> request.method >> request.path;

                std::string headerLine;
                while (std::getline(headerStream, headerLine)) {
                    if (!headerLine.empty() && headerLine.back() == '\r') {
                        headerLine.pop_back();
                    }
                    auto colon = headerLine.find(':');
                    if (colon != std::string::npos) {
                        std::string key = utils::trim(headerLine.substr(0, colon));
                        std::string value = utils::trim(headerLine.substr(colon + 1));
                        for (auto& c : key) c = std::tolower(static_cast<unsigned char>(c));
                        request.headers[key] = value;
                    }
                }

                auto it = request.headers.find("content-length");
                if (it != request.headers.end()) {
                    contentLength = static_cast<size_t>(std::stoul(it->second));
                }
            }
        }

        if (headerEnd != std::string::npos) {
            size_t bodyStart = headerEnd + 4;
            size_t currentBody = buffer.size() - bodyStart;
            if (currentBody >= contentLength) {
                request.body = buffer.substr(bodyStart, contentLength);
                break;
            }
        }
    }

    if (request.method.empty()) {
        throw std::runtime_error("Malformed HTTP request");
    }

    return request;
}

void HttpServer::handleGet(int clientFd, const Request& request) {
    std::string path = request.path == "/" ? "/index.html" : request.path;
    std::string content = readFile("public" + path);
    if (content.empty()) {
        content = readFile("../public" + path);
    }

    if (content.empty()) {
        sendResponse(clientFd, "HTTP/1.1 404 Not Found", "text/plain", "Not Found");
        return;
    }

    sendResponse(clientFd, "HTTP/1.1 200 OK", detectContentType(path), content);
}

void HttpServer::handlePost(int clientFd, const Request& request) {
    if (request.path != "/plot") {
        sendResponse(clientFd, "HTTP/1.1 404 Not Found", "text/plain", "Not Found");
        return;
    }

    auto form = parseForm(request.body);
    PlotRequest plotRequest;
    plotRequest.xs = utils::parseDoubles(form["xValues"]);
    plotRequest.ys = utils::parseDoubles(form["yValues"]);
    plotRequest.type = parsePlotType(form["plot"]);

    if (plotRequest.xs.empty() || plotRequest.ys.empty() || plotRequest.xs.size() != plotRequest.ys.size()) {
        sendResponse(clientFd, "HTTP/1.1 400 Bad Request", "text/plain", "Invalid coordinates");
        return;
    }

    PlotRenderer renderer(800, 600);
    try {
        Image image = renderer.render(plotRequest);
        std::string bmp = image.toBMP();
        sendBinaryResponse(clientFd, "HTTP/1.1 200 OK", "image/bmp", bmp);
    } catch (const std::exception& ex) {
        sendResponse(clientFd, "HTTP/1.1 500 Internal Server Error", "text/plain", ex.what());
    }
}

void HttpServer::sendResponse(int clientFd, const std::string& status, const std::string& contentType, const std::string& body) {
    std::ostringstream oss;
    oss << status << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "Cache-Control: no-store\r\n\r\n";
    oss << body;
    std::string response = oss.str();
    send(clientFd, response.data(), response.size(), 0);
}

void HttpServer::sendBinaryResponse(int clientFd, const std::string& status, const std::string& contentType, const std::string& data) {
    std::ostringstream oss;
    oss << status << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << data.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "Cache-Control: no-store\r\n\r\n";
    std::string header = oss.str();
    send(clientFd, header.data(), header.size(), 0);
    send(clientFd, data.data(), data.size(), 0);
}

std::unordered_map<std::string, std::string> HttpServer::parseForm(const std::string& body) {
    std::unordered_map<std::string, std::string> fields;
    for (const auto& kv : utils::split(body, '&')) {
        auto eq = kv.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = utils::urlDecode(kv.substr(0, eq));
        std::string value = utils::urlDecode(kv.substr(eq + 1));
        fields[key] = value;
    }
    return fields;
}

std::string HttpServer::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
