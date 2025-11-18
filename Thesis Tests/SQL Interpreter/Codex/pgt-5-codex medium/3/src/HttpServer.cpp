#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "Engine.h"

HttpServer::HttpServer(Engine &engine) : engine_(engine) {}

void HttpServer::start(uint16_t port) {
#ifdef _WIN32
    throw std::runtime_error("POSIX sockets required for HttpServer");
#else
    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Unable to create socket");
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(serverFd);
        throw std::runtime_error("Unable to bind to port");
    }

    if (listen(serverFd, 10) < 0) {
        close(serverFd);
        throw std::runtime_error("Unable to listen on socket");
    }

    std::cout << "HTTP server listening on http://localhost:" << port << std::endl;

    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);
        int clientSocket = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddress), &clientLen);
        if (clientSocket < 0) {
            continue;
        }
        handleClient(clientSocket);
        close(clientSocket);
    }

    close(serverFd);
#endif
}

void HttpServer::handleClient(int clientSocket) {
    std::string request = readRequest(clientSocket);
    if (request.empty()) {
        return;
    }

    std::istringstream stream(request);
    std::string method;
    std::string path;
    stream >> method >> path;

    if (method == "GET" && path == "/") {
        std::string body = buildHtmlPage();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html; charset=UTF-8\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << body;
        std::string payload = response.str();
        send(clientSocket, payload.c_str(), payload.size(), 0);
        return;
    }

    if (method == "POST" && path == "/execute") {
        std::string body = extractBody(request);
        std::string sql;
        const std::string prefix = "sql=";
        if (body.rfind(prefix, 0) == 0) {
            sql = urlDecode(body.substr(prefix.size()));
        } else {
            sql = urlDecode(body);
        }
        std::string result = engine_.executeStatementWeb(sql);
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain; charset=UTF-8\r\n";
        response << "Content-Length: " << result.size() << "\r\n";
        response << "Connection: close\r\n\r\n";
        response << result;
        std::string payload = response.str();
        send(clientSocket, payload.c_str(), payload.size(), 0);
        return;
    }

    const std::string notFound = "HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
    send(clientSocket, notFound.c_str(), notFound.size(), 0);
}

std::string HttpServer::buildHtmlPage() const {
    std::ostringstream page;
    page << R"HTML(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8" />
<title>Mini SQL</title>
<style>
body { font-family: Arial, sans-serif; margin: 2rem; }
textarea { width: 100%; height: 8rem; }
pre { background: #f4f4f4; padding: 1rem; border-radius: 4px; }
button { margin-top: 1rem; padding: 0.5rem 1rem; }
</style>
</head>
<body>
<h1>Mini SQL Interpreter</h1>
<textarea id="sql" placeholder="Write SQL statements here..."></textarea>
<button onclick="executeSql()">Execute</button>
<pre id="result"></pre>
<script>
async function executeSql() {
    const sql = document.getElementById('sql').value;
    const response = await fetch('/execute', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'sql=' + encodeURIComponent(sql)
    });
    const text = await response.text();
    document.getElementById('result').textContent = text;
}
</script>
</body>
</html>)HTML";
    return page.str();
}

std::string HttpServer::urlDecode(const std::string &value) const {
    std::string result;
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '+') {
            result.push_back(' ');
        } else if (c == '%' && i + 2 < value.size()) {
            std::string hex = value.substr(i + 1, 2);
            char decoded = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result.push_back(decoded);
            i += 2;
        } else {
            result.push_back(c);
        }
    }
    return result;
}

std::string HttpServer::readRequest(int clientSocket) const {
    std::string data;
    char buffer[4096];
    ssize_t bytesRead = 0;
    do {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            data.append(buffer, bytesRead);
            auto headerEnd = data.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                auto contentLengthHeader = getHeaderValue(data.substr(0, headerEnd), "Content-Length");
                if (contentLengthHeader.empty()) {
                    break;
                }
                size_t contentLength = static_cast<size_t>(std::stoul(contentLengthHeader));
                if (data.size() >= headerEnd + 4 + contentLength) {
                    break;
                }
            }
        }
    } while (bytesRead > 0);
    return data;
}

std::string HttpServer::extractBody(const std::string &request) const {
    auto position = request.find("\r\n\r\n");
    if (position == std::string::npos) {
        return std::string();
    }
    return request.substr(position + 4);
}

std::string HttpServer::getHeaderValue(const std::string &headers, const std::string &headerName) const {
    std::istringstream stream(headers);
    std::string line;
    std::string prefix = headerName + ":";
    while (std::getline(stream, line)) {
        if (line.rfind(prefix, 0) == 0) {
            std::string value = line.substr(prefix.size());
            size_t begin = value.find_first_not_of(" \t");
            if (begin == std::string::npos) {
                return std::string();
            }
            return value.substr(begin);
        }
    }
    return std::string();
}
