#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

HttpServer::HttpServer() = default;

namespace {

std::string httpResponse(int status, const std::string &statusText, const std::string &body, const std::string &contentType = "text/html") {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << ' ' << statusText << "\r\n";
    oss << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << body;
    return oss.str();
}

bool readRequest(int clientSocket, std::string &request) {
    char buffer[4096];
    ssize_t received = 0;
    while ((received = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, received);
        // Simple heuristic: stop when we saw CRLF CRLF and content-length satisfied
        if (request.find("\r\n\r\n") != std::string::npos) {
            auto pos = request.find("Content-Length:");
            if (pos == std::string::npos) {
                break;
            }
            size_t start = pos + 15;
            while (start < request.size() && request[start] == ' ') {
                ++start;
            }
            size_t end = request.find("\r\n", start);
            size_t length = std::stoul(request.substr(start, end - start));
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                size_t bodySize = request.size() - (bodyStart + 4);
                if (bodySize >= length) {
                    break;
                }
            }
        }
    }
    return !request.empty();
}

std::string getRequestPath(const std::string &request) {
    std::istringstream iss(request);
    std::string method, path;
    iss >> method >> path;
    return path;
}

std::string getRequestMethod(const std::string &request) {
    std::istringstream iss(request);
    std::string method;
    iss >> method;
    return method;
}

std::string getRequestBody(const std::string &request) {
    auto pos = request.find("\r\n\r\n");
    if (pos == std::string::npos) {
        return {};
    }
    return request.substr(pos + 4);
}

} // namespace

void HttpServer::start(unsigned short port, const std::function<std::string(const std::string &)> &handler) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSocket, 10) < 0) {
        close(serverSocket);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "HTTP server listening on port " << port << std::endl;

    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddress), &clientLen);
        if (clientSocket < 0) {
            continue;
        }

        std::string request;
        if (!readRequest(clientSocket, request)) {
            close(clientSocket);
            continue;
        }

        std::string method = getRequestMethod(request);
        std::string path = getRequestPath(request);
        std::string response;

        if (method == "GET" && path == "/") {
            response = httpResponse(200, "OK", buildHtmlPage());
        } else if (method == "POST" && path == "/execute") {
            std::string sql = getRequestBody(request);
            std::string result = handler(sql);
            response = httpResponse(200, "OK", result, "text/plain");
        } else {
            response = httpResponse(404, "Not Found", "Not found", "text/plain");
        }

        send(clientSocket, response.c_str(), response.size(), 0);
        close(clientSocket);
    }
}

std::string HttpServer::buildHtmlPage() const {
    std::string html = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8" />
<title>MiniSQL</title>
<style>
body { font-family: Arial, sans-serif; margin: 2rem; }
textarea { width: 100%; height: 150px; }
pre { background: #f5f5f5; padding: 1rem; }
</style>
</head>
<body>
<h1>MiniSQL</h1>
<textarea id="sql" placeholder="Enter SQL statements ending with semicolons"></textarea>
<br />
<button onclick="runSql()">Execute</button>
<pre id="output"></pre>
<script>
async function runSql() {
    const sql = document.getElementById('sql').value;
    const response = await fetch('/execute', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: sql
    });
    const text = await response.text();
    document.getElementById('output').textContent = text;
}
</script>
</body>
</html>
)HTML";
    return html;
}
