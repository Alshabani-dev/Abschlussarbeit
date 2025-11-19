#include "HttpServer.h"

#include "Utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

namespace {

constexpr int kBacklog = 16;
constexpr size_t kMaxRead = 4096;

std::string sanitizePath(const std::string &path) {
    if (path.empty() || path[0] != '/') {
        return {};
    }
    if (path == "/") {
        return "index.html";
    }
    std::string sanitized = path.substr(1);
    if (sanitized.find("..") != std::string::npos) {
        return {};
    }
    return sanitized;
}

std::string lowercase(const std::string &value) {
    std::string result = value;
    for (char &c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

}  // namespace

HttpServer::HttpServer(int port, const std::string &publicDir)
    : port_(port),
      publicDir_(publicDir),
      renderer_(800, 600),
      serverFd_(-1),
      running_(false) {}

void HttpServer::run() {
    serverFd_ = createServerSocket();
    if (serverFd_ < 0) {
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    running_ = true;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = accept(serverFd_, reinterpret_cast<sockaddr *>(&clientAddr), &addrLen);
        if (clientFd < 0) {
            if (!running_) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            std::perror("accept");
            continue;
        }
        handleClient(clientFd);
        close(clientFd);
    }

    if (serverFd_ >= 0) {
        close(serverFd_);
        serverFd_ = -1;
    }
    running_ = false;
}

int HttpServer::createServerSocket() {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::perror("setsockopt");
        close(serverFd);
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        close(serverFd);
        return -1;
    }

    if (listen(serverFd, kBacklog) < 0) {
        std::perror("listen");
        close(serverFd);
        return -1;
    }

    return serverFd;
}

void HttpServer::handleClient(int clientFd) {
    std::string raw = readRequest(clientFd);
    if (raw.empty()) {
        return;
    }

    HttpRequest request;
    if (!parseRequest(raw, request)) {
        sendResponse(clientFd, "400 Bad Request",
                     {{"Content-Length", "0"}, {"Connection", "close"}}, "");
        return;
    }

    if (request.method == "GET") {
        handleGet(clientFd, request);
    } else if (request.method == "POST") {
        handlePost(clientFd, request);
    } else {
        sendResponse(clientFd, "405 Method Not Allowed",
                     {{"Content-Length", "0"}, {"Connection", "close"}}, "");
    }
}

std::string HttpServer::readRequest(int clientFd) {
    std::string data;
    data.reserve(1024);
    ssize_t bytes = 0;
    size_t headerEnd = std::string::npos;
    size_t contentLength = 0;

    while (true) {
        char buffer[kMaxRead];
        bytes = recv(clientFd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }
        data.append(buffer, buffer + bytes);

        if (headerEnd == std::string::npos) {
            headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                std::string headerPart = data.substr(0, headerEnd);
                std::istringstream stream(headerPart);
                std::string line;
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    auto colon = line.find(':');
                    if (colon == std::string::npos) {
                        continue;
                    }
                    std::string key = lowercase(line.substr(0, colon));
                    std::string value = utils::trim(line.substr(colon + 1));
                    if (key == "content-length") {
                        contentLength = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
                    }
                }
            }
        }

        if (headerEnd != std::string::npos) {
            size_t totalSize = headerEnd + 4 + contentLength;
            if (data.size() >= totalSize) {
                break;
            }
        }

        if (data.size() > 1000000) {
            break;
        }
    }

    return data;
}

bool HttpServer::parseRequest(const std::string &raw, HttpRequest &request) {
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false;
    }

    std::string headerPart = raw.substr(0, headerEnd);
    std::string bodyPart = raw.substr(headerEnd + 4);

    std::istringstream stream(headerPart);
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
        auto colon = headerLine.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = headerLine.substr(0, colon);
        std::string value = utils::trim(headerLine.substr(colon + 1));
        request.headers[key] = value;
    }

    request.body = bodyPart;
    return true;
}

std::map<std::string, std::string> HttpServer::parseFormData(const std::string &body) {
    std::map<std::string, std::string> fields;
    size_t start = 0;
    while (start < body.size()) {
        size_t end = body.find('&', start);
        if (end == std::string::npos) {
            end = body.size();
        }
        std::string pair = body.substr(start, end - start);
        size_t equals = pair.find('=');
        if (equals != std::string::npos) {
            std::string key = utils::urlDecode(pair.substr(0, equals));
            std::string value = utils::urlDecode(pair.substr(equals + 1));
            fields[key] = value;
        }
        start = end + 1;
    }
    return fields;
}

void HttpServer::handleGet(int clientFd, const HttpRequest &request) {
    std::string relative = sanitizePath(request.path);
    if (relative.empty()) {
        sendResponse(clientFd, "404 Not Found", {{"Content-Length", "0"}, {"Connection", "close"}}, "");
        return;
    }

    const std::string candidates[] = {
        publicDir_ + "/" + relative,
        "../" + publicDir_ + "/" + relative,
    };

    std::string content;
    for (const auto &path : candidates) {
        content = utils::readFile(path);
        if (!content.empty()) {
            break;
        }
    }

    if (content.empty()) {
        sendResponse(clientFd, "404 Not Found", {{"Content-Length", "0"}, {"Connection", "close"}}, "");
        return;
    }

    std::map<std::string, std::string> headers = {
        {"Content-Type", getContentType(relative)},
        {"Content-Length", std::to_string(content.size())},
        {"Connection", "close"},
        {"Cache-Control", "no-store"},
    };

    sendResponse(clientFd, "200 OK", headers, content);
}

void HttpServer::handlePost(int clientFd, const HttpRequest &request) {
    if (request.path != "/plot") {
        sendResponse(clientFd, "404 Not Found", {{"Content-Length", "0"}, {"Connection", "close"}}, "");
        return;
    }

    auto fields = parseFormData(request.body);
    auto xIt = fields.find("xValues");
    auto yIt = fields.find("yValues");
    auto plotIt = fields.find("plot");

    if (xIt == fields.end() || yIt == fields.end() || plotIt == fields.end()) {
        sendResponse(clientFd, "400 Bad Request", {{"Content-Length", "0"}, {"Connection", "close"}}, "");
        return;
    }

    std::vector<double> xs;
    std::vector<double> ys;
    if (!utils::parseDoubleList(xIt->second, xs) || !utils::parseDoubleList(yIt->second, ys) || xs.size() != ys.size()) {
        sendResponse(clientFd, "400 Bad Request", {{"Content-Length", "0"}, {"Connection", "close"}}, "");
        return;
    }

    std::string plotType = plotIt->second;
    Image image(1, 1);
    if (plotType == "line") {
        image = renderer_.renderLinePlot(xs, ys);
    } else if (plotType == "scatter") {
        image = renderer_.renderScatterPlot(xs, ys);
    } else if (plotType == "bar") {
        image = renderer_.renderBarPlot(xs, ys);
    } else {
        sendResponse(clientFd, "400 Bad Request", {{"Content-Length", "0"}, {"Connection", "close"}}, "");
        return;
    }

    std::string bmp = image.toBMP();
    std::map<std::string, std::string> headers = {
        {"Content-Type", "image/bmp"},
        {"Content-Length", std::to_string(bmp.size())},
        {"Connection", "close"},
        {"Cache-Control", "no-store"},
    };

    sendResponse(clientFd, "200 OK", headers, bmp);
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
    return "text/plain";
}

void HttpServer::sendResponse(int clientFd, const std::string &status,
                              const std::map<std::string, std::string> &headers,
                              const std::string &body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    for (const auto &entry : headers) {
        response << entry.first << ": " << entry.second << "\r\n";
    }
    response << "\r\n";
    std::string headerData = response.str();

    send(clientFd, headerData.data(), headerData.size(), 0);
    if (!body.empty()) {
        send(clientFd, body.data(), body.size(), 0);
    }
}

void HttpServer::stop() {
    running_ = false;
    if (serverFd_ >= 0) {
        shutdown(serverFd_, SHUT_RDWR);
    }
}
