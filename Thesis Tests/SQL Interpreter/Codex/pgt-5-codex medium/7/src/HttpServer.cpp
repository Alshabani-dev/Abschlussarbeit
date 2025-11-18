#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "Engine.h"
#include "Utils.h"

namespace {

const char *kIndexHtml = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Minimal SQL Interpreter</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        textarea { width: 100%; height: 120px; }
        pre { background: #f5f5f5; padding: 16px; }
        button { padding: 8px 16px; margin-top: 8px; }
    </style>
</head>
<body>
    <h1>Minimal SQL Interpreter</h1>
    <textarea id="sql" placeholder="Enter SQL statements..."></textarea><br/>
    <button onclick="runQuery()">Run</button>
    <pre id="result"></pre>
    <script>
        async function runQuery() {
            const sql = document.getElementById('sql').value;
            const response = await fetch('/execute', {
                method: 'POST',
                headers: { 'Content-Type': 'text/plain' },
                body: sql
            });
            const text = await response.text();
            document.getElementById('result').innerText = text;
        }
    </script>
</body>
</html>
)HTML";

std::string getRequestLine(const std::string &request) {
    size_t end = request.find("\r\n");
    if (end == std::string::npos) {
        return "";
    }
    return request.substr(0, end);
}

std::string getHeaderValue(const std::string &headers, const std::string &name) {
    std::string upperName = toLower(name);
    size_t start = 0;
    while (start < headers.size()) {
        size_t end = headers.find("\r\n", start);
        std::string line = headers.substr(start, end - start);
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string headerName = toLower(trim(line.substr(0, colon)));
            if (headerName == upperName) {
                return trim(line.substr(colon + 1));
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 2;
    }
    return "";
}

} // namespace

HttpServer::HttpServer(Engine &engine) : engine_(engine) {}

void HttpServer::start(int port) {
    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    int enable = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Failed to bind to port " << port << "\n";
        close(serverFd);
        return;
    }

    if (listen(serverFd, 8) < 0) {
        std::cerr << "Failed to listen on port\n";
        close(serverFd);
        return;
    }

    std::cout << "Serving web UI on http://localhost:" << port << "\n";
    while (true) {
        int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0) {
            continue;
        }
        handleClient(clientFd);
        close(clientFd);
    }
}

void HttpServer::handleClient(int clientFd) {
    std::string request;
    char buffer[4096];
    ssize_t received;
    while ((received = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, buffer + received);
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    if (request.empty()) {
        return;
    }

    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return;
    }

    std::string headers = request.substr(0, headerEnd);
    std::string requestLine = getRequestLine(request);
    std::istringstream ss(requestLine);
    std::string method;
    std::string path;
    ss >> method >> path;

    size_t contentLength = 0;
    std::string contentLengthStr = getHeaderValue(headers, "Content-Length");
    if (!contentLengthStr.empty()) {
        contentLength = static_cast<size_t>(std::stoul(contentLengthStr));
    }

    std::string body = request.substr(headerEnd + 4);
    while (body.size() < contentLength) {
        received = recv(clientFd, buffer, sizeof(buffer), 0);
        if (received <= 0) break;
        body.append(buffer, buffer + received);
    }

    if (method == "GET" && path == "/") {
        sendResponse(clientFd, "200 OK", "text/html", kIndexHtml);
        return;
    }
    if (method == "POST" && path == "/execute") {
        std::string sql = body;
        std::string output = engine_.executeStatementWeb(sql);
        sendResponse(clientFd, "200 OK", "text/plain", output);
        return;
    }
    sendResponse(clientFd, "404 Not Found", "text/plain", "Not Found");
}

void HttpServer::sendResponse(int clientFd, const std::string &status, const std::string &contentType, const std::string &body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << body;
    std::string response = oss.str();
    send(clientFd, response.data(), response.size(), 0);
}
