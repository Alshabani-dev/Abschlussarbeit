#include "HttpServer.h"

#include <arpa/inet.h>
#include <cctype>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

bool readRequest(int clientFd, std::string &request) {
    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];
    while (true) {
        ssize_t bytes = recv(clientFd, buffer, bufferSize, 0);
        if (bytes <= 0) {
            return false;
        }
        request.append(buffer, static_cast<size_t>(bytes));
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            continue;
        }
        size_t contentLength = 0;
        std::string headers = request.substr(0, headerEnd);
        auto pos = headers.find("Content-Length:");
        if (pos != std::string::npos) {
            pos += std::strlen("Content-Length:");
            while (pos < headers.size() && std::isspace(static_cast<unsigned char>(headers[pos]))) {
                ++pos;
            }
            size_t end = headers.find("\r\n", pos);
            std::string lengthStr = headers.substr(pos, end - pos);
            try {
                contentLength = static_cast<size_t>(std::stoul(lengthStr));
            } catch (...) {
                contentLength = 0;
            }
        }
        size_t totalLength = headerEnd + 4 + contentLength;
        if (request.size() >= totalLength) {
            return true;
        }
        if (contentLength == 0) {
            return true;
        }
    }
}

} // namespace

void HttpServer::serve(int port, Handler handler) {
    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::perror("socket");
        return;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::perror("setsockopt");
        close(serverFd);
        return;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        std::perror("bind");
        close(serverFd);
        return;
    }

    if (listen(serverFd, 10) < 0) {
        std::perror("listen");
        close(serverFd);
        return;
    }

    std::cout << "HTTP server listening on port " << port << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            std::perror("accept");
            continue;
        }

        std::string request;
        if (!readRequest(clientFd, request)) {
            close(clientFd);
            continue;
        }

        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            close(clientFd);
            continue;
        }

        std::istringstream requestLineStream(request.substr(0, request.find("\r\n")));
        std::string method;
        std::string path;
        std::string version;
        requestLineStream >> method >> path >> version;

        std::string response;
        if (method == "GET" && path == "/") {
            response = buildHttpResponse(renderHtml(), "text/html; charset=utf-8");
        } else if (method == "POST" && path == "/execute") {
            std::string headers = request.substr(0, headerEnd);
            size_t contentLength = 0;
            auto pos = headers.find("Content-Length:");
            if (pos != std::string::npos) {
                pos += std::strlen("Content-Length:");
                while (pos < headers.size() && std::isspace(static_cast<unsigned char>(headers[pos]))) {
                    ++pos;
                }
                size_t end = headers.find("\r\n", pos);
                std::string lengthStr = headers.substr(pos, end - pos);
                try {
                    contentLength = static_cast<size_t>(std::stoul(lengthStr));
                } catch (...) {
                    contentLength = 0;
                }
            }
            std::string body = request.substr(headerEnd + 4);
            if (body.size() > contentLength && contentLength > 0) {
                body = body.substr(0, contentLength);
            }
            std::string result = handler(body);
            response = buildHttpResponse(result, "text/plain; charset=utf-8");
        } else {
            std::string body = "Not found";
            std::ostringstream out;
            out << "HTTP/1.1 404 Not Found\r\n"
                << "Content-Type: text/plain; charset=utf-8\r\n"
                << "Content-Length: " << body.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << body;
            response = out.str();
        }

        send(clientFd, response.c_str(), response.size(), 0);
        close(clientFd);
    }

    close(serverFd);
}

std::string HttpServer::buildHttpResponse(const std::string &body, const std::string &contentType) const {
    std::ostringstream out;
    out << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << body;
    return out.str();
}

std::string HttpServer::renderHtml() const {
    return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>MiniSQL</title>
<style>
body { font-family: Arial, sans-serif; margin: 2rem; }
textarea { width: 100%; height: 150px; }
button { margin-top: 1rem; padding: 0.5rem 1rem; }
pre { background: #111; color: #0f0; padding: 1rem; min-height: 200px; white-space: pre-wrap; }
</style>
</head>
<body>
<h1>MiniSQL Interpreter</h1>
<textarea id="sql" placeholder="Enter SQL here..."></textarea>
<div>
    <button onclick="runQuery()">Run</button>
</div>
<pre id="output"></pre>
<script>
async function runQuery() {
    const sql = document.getElementById('sql').value;
    const response = await fetch('/execute', {
        method: 'POST',
        body: sql
    });
    const text = await response.text();
    document.getElementById('output').textContent = text;
}
</script>
</body>
</html>
)HTML";
}
