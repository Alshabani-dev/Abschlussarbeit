#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

namespace {

std::string readRequest(int clientFd) {
    std::string request;
    char buffer[2048];
    while (true) {
        ssize_t bytes = recv(clientFd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }
        request.append(buffer, bytes);
        auto headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            continue;
        }
        std::string headers = request.substr(0, headerEnd);
        size_t contentLength = 0;
        std::istringstream iss(headers);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.rfind("Content-Length:", 0) == 0) {
                contentLength = static_cast<size_t>(std::stoul(line.substr(15)));
                break;
            }
        }
        size_t bodySize = request.size() - (headerEnd + 4);
        if (bodySize >= contentLength) {
            break;
        }
    }
    return request;
}

void sendResponse(int clientFd, const std::string &body, const std::string &contentType) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << body;
    std::string response = oss.str();
    send(clientFd, response.c_str(), response.size(), 0);
}

const char *kHtmlPage = R"HTML(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8" />
<title>Minimal SQL Interpreter</title>
<style>
body { font-family: sans-serif; margin: 2rem; background: #f7f7f7; }
textarea { width: 100%; height: 120px; font-family: monospace; }
pre { background: #222; color: #eee; padding: 1rem; min-height: 150px; }
button { padding: 0.5rem 1rem; font-size: 1rem; }
</style>
</head>
<body>
<h1>Minimal SQL Interpreter</h1>
<textarea id="sql" placeholder="Enter SQL here..."></textarea><br/>
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
</html>)HTML";

} // namespace

void HttpServer::start(unsigned short port, const std::function<std::string(const std::string &)> &handler) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::perror("socket");
        return;
    }
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        close(serverFd);
        return;
    }

    if (listen(serverFd, 8) < 0) {
        std::perror("listen");
        close(serverFd);
        return;
    }

    std::cout << "HTTP server running on http://localhost:" << port << "\n";
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            continue;
        }
        std::string request = readRequest(clientFd);
        if (request.rfind("GET", 0) == 0) {
            sendResponse(clientFd, kHtmlPage, "text/html; charset=utf-8");
        } else if (request.rfind("POST /execute", 0) == 0) {
            auto separator = request.find("\r\n\r\n");
            std::string body;
            if (separator != std::string::npos) {
                body = request.substr(separator + 4);
            }
            std::string result = handler(body);
            sendResponse(clientFd, result, "text/plain; charset=utf-8");
        } else {
            const std::string notFound = "Not Found";
            std::ostringstream oss;
            oss << "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: " << notFound.size()
                << "\r\nConnection: close\r\n\r\n"
                << notFound;
            std::string response = oss.str();
            send(clientFd, response.c_str(), response.size(), 0);
        }
        close(clientFd);
    }
    close(serverFd);
}
