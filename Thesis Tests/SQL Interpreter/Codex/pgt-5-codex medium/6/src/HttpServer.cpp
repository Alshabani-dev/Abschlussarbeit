#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

#include "Engine.h"
#include "Utils.h"

namespace {

const char *kIndexPage = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8" />
<title>Minimal SQL Interpreter</title>
<style>
body { font-family: Arial, sans-serif; margin: 20px; }
textarea { width: 100%; height: 150px; }
pre { background: #f5f5f5; padding: 10px; border: 1px solid #ccc; }
button { padding: 10px 20px; }
</style>
</head>
<body>
<h1>Minimal SQL Interpreter</h1>
<textarea id="sqlInput" placeholder="Enter SQL here..."></textarea><br/>
<button onclick="runQuery()">Run</button>
<h3>Output</h3>
<pre id="output"></pre>
<script>
async function runQuery() {
    const sql = document.getElementById('sqlInput').value;
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

} // namespace

HttpServer::HttpServer(Engine &engine, int port) : engine_(engine), port_(port) {}

void HttpServer::start() {
    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        close(serverFd);
        return;
    }

    if (listen(serverFd, 10) < 0) {
        std::cerr << "Failed to listen on port " << port_ << std::endl;
        close(serverFd);
        return;
    }

    std::cout << "HTTP server listening on port " << port_ << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            continue;
        }
        handleClient(clientSocket);
        close(clientSocket);
    }
}

void HttpServer::handleClient(int clientSocket) {
    std::string request;
    char buffer[4096];
    while (true) {
        ssize_t bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            return;
        }
        request.append(buffer, bytes);
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            continue;
        }

        std::string headerPart = request.substr(0, headerEnd);
        std::string body = request.substr(headerEnd + 4);

        size_t contentLength = 0;
        std::istringstream headerStream(headerPart);
        std::string requestLine;
        std::getline(headerStream, requestLine);
        std::string headerLine;
        while (std::getline(headerStream, headerLine)) {
            if (!headerLine.empty() && headerLine.back() == '\r') {
                headerLine.pop_back();
            }
            std::string lower = Utils::toLower(headerLine);
            const std::string key = "content-length:";
            if (lower.rfind(key, 0) == 0) {
                std::string number = Utils::trim(headerLine.substr(key.size()));
                contentLength = static_cast<size_t>(std::stoul(number));
                break;
            }
        }

        while (body.size() < contentLength) {
            ssize_t more = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (more <= 0) {
                break;
            }
            body.append(buffer, more);
        }

        std::istringstream requestLineStream(requestLine);
        std::string method;
        std::string path;
        std::string version;
        requestLineStream >> method >> path >> version;
        if (method.empty()) {
            return;
        }

        std::string response = handleRequest(method, path, body);
        send(clientSocket, response.c_str(), response.size(), 0);
        break;
    }
}

std::string HttpServer::buildHttpResponse(const std::string &body,
                                          const std::string &contentType,
                                          const std::string &status) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << body;
    return oss.str();
}

std::string HttpServer::handleRequest(const std::string &method,
                                      const std::string &path,
                                      const std::string &body) {
    if (method == "GET" && path == "/") {
        return buildHttpResponse(kIndexPage, "text/html");
    }
    if (method == "POST" && path == "/execute") {
        std::string result = engine_.executeStatementWeb(body);
        return buildHttpResponse(result, "text/plain");
    }
    return buildHttpResponse("Not Found", "text/plain", "404 Not Found");
}
