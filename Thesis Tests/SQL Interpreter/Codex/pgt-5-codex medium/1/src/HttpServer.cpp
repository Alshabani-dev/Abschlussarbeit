#include "HttpServer.h"

#include "Engine.h"

#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
void closeSocket(int socket) { close(socket); }
}

HttpServer::HttpServer(Engine &engine) : engine_(engine) {}

void HttpServer::start(int port) {
    int serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error("Unable to create socket");
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        closeSocket(serverSocket);
        throw std::runtime_error("Unable to bind socket");
    }

    if (listen(serverSocket, 8) < 0) {
        closeSocket(serverSocket);
        throw std::runtime_error("Unable to listen on socket");
    }

    std::cout << "Web interface available at http://localhost:" << port << std::endl;

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddr), &len);
        if (clientSocket < 0) {
            continue;
        }
        handleClient(clientSocket);
        closeSocket(clientSocket);
    }
}

void HttpServer::handleClient(int clientSocket) {
    std::string request;
    char buffer[4096];
    while (true) {
        int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }
        request.append(buffer, bytes);
        auto headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            std::size_t bodyStart = headerEnd + 4;
            std::string headers = request.substr(0, headerEnd);
            bool isPost = headers.rfind("POST", 0) == 0;
            std::size_t contentLength = 0;
            if (isPost) {
                auto pos = headers.find("Content-Length:");
                if (pos != std::string::npos) {
                    pos += 15;
                    while (pos < headers.size() && std::isspace(static_cast<unsigned char>(headers[pos]))) {
                        ++pos;
                    }
                    std::size_t end = pos;
                    while (end < headers.size() && std::isdigit(static_cast<unsigned char>(headers[end]))) {
                        ++end;
                    }
                    contentLength = std::stoul(headers.substr(pos, end - pos));
                }
            }
            if (request.size() >= bodyStart + contentLength) {
                break;
            }
        }
    }

    if (request.empty()) {
        return;
    }

    auto headerEnd = request.find("\r\n\r\n");
    std::string headerSection = headerEnd == std::string::npos ? request : request.substr(0, headerEnd);
    std::string body = headerEnd == std::string::npos ? std::string() : request.substr(headerEnd + 4);

    std::istringstream headerStream(headerSection);
    std::string requestLine;
    std::getline(headerStream, requestLine);
    std::istringstream requestLineStream(requestLine);
    std::string method;
    std::string path;
    requestLineStream >> method >> path;

    std::string responseBody;
    std::string contentType;
    std::string statusLine = "HTTP/1.1 200 OK\r\n";

    if (method == "POST") {
        if (path != "/execute") {
            statusLine = "HTTP/1.1 404 Not Found\r\n";
            responseBody = "Unknown endpoint";
            contentType = "text/plain; charset=utf-8";
        } else {
            responseBody = engine_.executeStatementWeb(body);
            contentType = "text/plain; charset=utf-8";
        }
    } else {
        responseBody = buildHtmlPage();
        contentType = "text/html; charset=utf-8";
    }

    std::ostringstream response;
    response << statusLine;
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << responseBody.size() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << responseBody;
    auto payload = response.str();
    send(clientSocket, payload.data(), static_cast<int>(payload.size()), 0);
}

std::string HttpServer::buildHtmlPage() const {
    return R"HTML(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Minimal SQL Interpreter</title>
<style>
body { font-family: sans-serif; margin: 2rem; background: #f7f7f7; }
textarea { width: 100%; height: 150px; }
pre { background: #fff; padding: 1rem; border: 1px solid #ccc; min-height: 150px; overflow: auto; }
button { padding: 0.5rem 1rem; margin-top: 1rem; }
</style>
</head>
<body>
<h1>Minimal SQL Interpreter</h1>
<textarea id="sql" placeholder="Enter SQL statements here..."></textarea>
<br/>
<button onclick="runQuery()">Run</button>
<pre id="output"></pre>
<script>
async function runQuery() {
  const sql = document.getElementById('sql').value;
  const response = await fetch('/execute', { method: 'POST', body: sql });
  const text = await response.text();
  document.getElementById('output').textContent = text;
}
</script>
</body>
</html>)HTML";
}
