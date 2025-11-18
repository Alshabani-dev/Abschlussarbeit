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

namespace {

std::string trim(const std::string &text) {
    std::size_t start = text.find_first_not_of(" \t\r\n");
    std::size_t end = text.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    return text.substr(start, end - start + 1);
}

std::size_t parseContentLength(const std::string &headers) {
    std::istringstream stream(headers);
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        const std::string prefix = "Content-Length:";
        if (line.size() >= prefix.size()) {
            if (std::equal(prefix.begin(), prefix.end(), line.begin(),
                           [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
                std::string value = trim(line.substr(prefix.size()));
                try {
                    return static_cast<std::size_t>(std::stoul(value));
                } catch (...) {
                    return 0;
                }
            }
        }
    }
    return 0;
}

} // namespace

HttpServer::HttpServer(Engine &engine) : engine_(engine) {}

void HttpServer::run(int port) {
    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    int enable = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd, 10) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Web UI running at http://localhost:" << port << '\n';

    while (true) {
        int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0) {
            continue;
        }

        std::string request;
        char buffer[4096];
        ssize_t received;
        std::size_t contentLength = 0;
        while ((received = recv(clientFd, buffer, sizeof(buffer), 0)) > 0) {
            request.append(buffer, received);
            std::size_t headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                if (contentLength == 0) {
                    contentLength = parseContentLength(request.substr(0, headerEnd));
                }
                if (request.size() >= headerEnd + 4 + contentLength) {
                    break;
                }
            }
            if (received < static_cast<ssize_t>(sizeof(buffer))) {
                break;
            }
        }

        std::string responseBody;
        std::string responseHeaders;
        std::string statusLine = "HTTP/1.1 200 OK\r\n";

        std::size_t headerEnd = request.find("\r\n\r\n");
        std::string headers = headerEnd == std::string::npos ? request : request.substr(0, headerEnd);
        std::string body;
        if (headerEnd != std::string::npos && request.size() >= headerEnd + 4) {
            body = request.substr(headerEnd + 4, contentLength);
        }

        std::istringstream headerStream(headers);
        std::string requestLine;
        std::getline(headerStream, requestLine);
        std::istringstream lineStream(requestLine);
        std::string method;
        std::string path;
        lineStream >> method >> path;

        if (method == "GET" || method == "HEAD") {
            responseBody = buildHtml();
            responseHeaders = "Content-Type: text/html; charset=utf-8\r\n";
        } else if (method == "POST" && path == "/execute") {
            std::string executionResult = engine_.executeStatementWeb(body);
            responseBody = executionResult;
            responseHeaders = "Content-Type: text/plain; charset=utf-8\r\n";
        } else {
            statusLine = "HTTP/1.1 404 Not Found\r\n";
            responseBody = "Not Found";
            responseHeaders = "Content-Type: text/plain; charset=utf-8\r\n";
        }

        std::ostringstream response;
        response << statusLine
                 << "Content-Length: " << responseBody.size() << "\r\n"
                 << "Connection: close\r\n"
                 << responseHeaders
                 << "\r\n";
        if (method != "HEAD") {
            response << responseBody;
        }
        auto responseStr = response.str();
        send(clientFd, responseStr.c_str(), responseStr.size(), 0);
        close(clientFd);
    }
}

std::string HttpServer::buildHtml() const {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>MiniSQL</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(135deg, #1f1c2c, #928dab);
      color: #fff;
      margin: 0;
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    .container {
      width: 90%;
      max-width: 900px;
      background: rgba(0,0,0,0.4);
      border-radius: 12px;
      padding: 24px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.5);
    }
    textarea {
      width: 100%;
      min-height: 150px;
      border-radius: 8px;
      border: none;
      padding: 16px;
      font-size: 16px;
      font-family: 'Courier New', monospace;
    }
    button {
      margin-top: 12px;
      padding: 12px 24px;
      border: none;
      border-radius: 8px;
      background: #ff7e5f;
      color: #fff;
      font-size: 16px;
      cursor: pointer;
    }
    button:hover {
      background: #feb47b;
    }
    pre {
      background: rgba(0,0,0,0.6);
      padding: 16px;
      border-radius: 8px;
      min-height: 180px;
      white-space: pre-wrap;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>MiniSQL Web Console</h1>
    <textarea id="sql" placeholder="Enter SQL commands here. Use Ctrl+Enter to execute."></textarea>
    <button onclick="runSql()">Run</button>
    <pre id="output">Awaiting query...</pre>
  </div>
  <script>
    document.getElementById('sql').addEventListener('keydown', function(event) {
      if (event.key === 'Enter' && event.ctrlKey) {
        event.preventDefault();
        runSql();
      }
    });
    function runSql() {
      const sql = document.getElementById('sql').value;
      fetch('/execute', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: sql
      })
      .then(response => response.text())
      .then(text => document.getElementById('output').textContent = text)
      .catch(err => document.getElementById('output').textContent = 'Error: ' + err);
    }
  </script>
</body>
</html>)HTML";
}
