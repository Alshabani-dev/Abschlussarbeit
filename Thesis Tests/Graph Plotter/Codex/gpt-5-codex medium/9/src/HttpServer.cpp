#include "HttpServer.h"

#include "Utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cstring>
#include <sstream>
#include <stdexcept>

HttpServer::HttpServer(int port)
    : port_(port), running_(true), renderer_(800, 600) {}

int HttpServer::createServerSocket() const {
    const int serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSocket, 10) < 0) {
        close(serverSocket);
        throw std::runtime_error("Failed to listen on socket");
    }

    return serverSocket;
}

std::string HttpServer::readRequest(int clientSocket) const {
    std::string data;
    char buffer[4096];
    ssize_t bytesRead;
    size_t contentLength = 0;
    bool headersParsed = false;

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        data.append(buffer, static_cast<size_t>(bytesRead));
        if (!headersParsed) {
            const size_t headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                headersParsed = true;
                const std::string headers = data.substr(0, headerEnd);
                const std::string search = "Content-Length:";
                const size_t pos = headers.find(search);
                if (pos != std::string::npos) {
                    size_t start = pos + search.size();
                    while (start < headers.size() &&
                           (headers[start] == ' ' || headers[start] == '\t')) {
                        ++start;
                    }
                    size_t end = start;
                    while (end < headers.size() && std::isdigit(headers[end])) {
                        ++end;
                    }
                    contentLength = static_cast<size_t>(std::stoul(headers.substr(start, end - start)));
                }
                const size_t totalSize = headerEnd + 4 + contentLength;
                while (data.size() < totalSize) {
                    bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                    if (bytesRead <= 0) {
                        break;
                    }
                    data.append(buffer, static_cast<size_t>(bytesRead));
                }
                break;
            }
        } else {
            break;
        }
    }
    return data;
}

std::unordered_map<std::string, std::string> HttpServer::parseFormData(const std::string& body) const {
    std::unordered_map<std::string, std::string> fields;
    size_t start = 0;
    while (start < body.size()) {
        size_t end = body.find('&', start);
        if (end == std::string::npos) {
            end = body.size();
        }
        const std::string pair = body.substr(start, end - start);
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = utils::urlDecode(pair.substr(0, eq));
            std::string value = utils::urlDecode(pair.substr(eq + 1));
            fields[key] = value;
        }
        start = end + 1;
    }
    return fields;
}

std::string HttpServer::getContentType(const std::string& path) const {
    const auto endsWith = [](const std::string& value, const std::string& suffix) {
        return value.size() >= suffix.size() &&
               value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    if (endsWith(path, ".html")) return "text/html; charset=utf-8";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    if (endsWith(path, ".bmp")) return "image/bmp";
    return "text/plain; charset=utf-8";
}

std::string HttpServer::serveStaticFile(const std::string& path, std::string& contentType) const {
    std::string relativePath = path;
    if (relativePath == "/") {
        relativePath = "/index.html";
    }
    if (relativePath.find("..") != std::string::npos) {
        return {};
    }
    contentType = getContentType(relativePath);

    const std::string primary = "public" + relativePath;
    std::string content = utils::readFile(primary);
    if (content.empty()) {
        const std::string secondary = "../" + primary;
        content = utils::readFile(secondary);
    }
    return content;
}

std::string HttpServer::handlePlotRequest(const std::string& body, std::string& contentType) const {
    auto fields = parseFormData(body);
    const auto plotIt = fields.find("plot");
    const auto xIt = fields.find("xValues");
    const auto yIt = fields.find("yValues");
    if (plotIt == fields.end() || xIt == fields.end() || yIt == fields.end()) {
        return {};
    }
    const std::vector<double> xs = utils::parseNumberList(xIt->second);
    const std::vector<double> ys = utils::parseNumberList(yIt->second);
    Image image = renderer_.render(xs, ys, plotIt->second);
    contentType = "image/bmp";
    return image.toBMP();
}

std::string HttpServer::buildResponse(const std::string& status,
                                      const std::string& contentType,
                                      const std::string& body,
                                      bool binary) const {
    std::ostringstream headers;
    headers << "HTTP/1.1 " << status << "\r\n";
    headers << "Content-Type: " << contentType << "\r\n";
    headers << "Content-Length: " << body.size() << "\r\n";
    headers << "Connection: close\r\n";
    if (binary) {
        headers << "Cache-Control: no-store\r\n";
    }
    headers << "\r\n";
    std::string response = headers.str();
    response.append(body);
    return response;
}

void HttpServer::handleClient(int clientSocket) const {
    std::string request = readRequest(clientSocket);
    if (request.empty()) {
        close(clientSocket);
        return;
    }
    const size_t lineEnd = request.find("\r\n");
    if (lineEnd == std::string::npos) {
        close(clientSocket);
        return;
    }
    std::istringstream requestLine(request.substr(0, lineEnd));
    std::string method;
    std::string path;
    requestLine >> method >> path;

    std::string contentType = "text/plain; charset=utf-8";
    std::string body;
    std::string status = "200 OK";
    bool binary = false;

    if (method == "GET") {
        body = serveStaticFile(path, contentType);
        if (body.empty()) {
            status = "404 Not Found";
            contentType = "text/plain; charset=utf-8";
            body = "File not found";
        }
    } else if (method == "POST" && path == "/plot") {
        const size_t separator = request.find("\r\n\r\n");
        const std::string payload = (separator != std::string::npos) ? request.substr(separator + 4) : "";
        body = handlePlotRequest(payload, contentType);
        if (body.empty()) {
            status = "400 Bad Request";
            contentType = "text/plain; charset=utf-8";
            body = "Invalid input";
        } else {
            binary = true;
        }
    } else {
        status = "405 Method Not Allowed";
        contentType = "text/plain; charset=utf-8";
        body = "Unsupported request";
    }

    const std::string response = buildResponse(status, contentType, body, binary);
    send(clientSocket, response.data(), response.size(), 0);
    close(clientSocket);
}

void HttpServer::run() {
    const int serverSocket = createServerSocket();
    while (running_) {
        sockaddr_in clientAddress {};
        socklen_t clientLen = sizeof(clientAddress);
        const int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientLen);
        if (clientSocket < 0) {
            continue;
        }
        handleClient(clientSocket);
    }
    close(serverSocket);
}
