#include "HttpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "Engine.h"
#include "Utils.h"

HttpServer::HttpServer(Engine &engine) : engine_(engine) {}

void HttpServer::run(int port) {
    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd, 16) < 0) {
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    std::cout << "Web server running on http://localhost:" << port << '\n';

    while (true) {
        int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0) {
            std::cerr << "Failed to accept connection" << '\n';
            continue;
        }
        handleClient(clientFd);
        close(clientFd);
    }

    close(serverFd);
}

void HttpServer::handleClient(int clientFd) {
    std::string request;
    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];
    ssize_t bytesRead;
    size_t headerEndPos = std::string::npos;
    size_t expectedBody = 0;
    bool headerParsed = false;

    while ((bytesRead = recv(clientFd, buffer, bufferSize, 0)) > 0) {
        request.append(buffer, static_cast<size_t>(bytesRead));
        if (!headerParsed) {
            headerEndPos = request.find("\r\n\r\n");
            if (headerEndPos != std::string::npos) {
                headerParsed = true;
                std::istringstream headerStream(request.substr(0, headerEndPos));
                std::string line;
                while (std::getline(headerStream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    auto colon = line.find(':');
                    if (colon != std::string::npos) {
                        std::string key = Utils::toLower(Utils::trim(line.substr(0, colon)));
                        std::string value = Utils::trim(line.substr(colon + 1));
                        if (key == "content-length") {
                            expectedBody = static_cast<size_t>(std::stoul(value));
                        }
                    }
                }
            }
        }
        if (headerParsed) {
            if (request.size() >= headerEndPos + 4 + expectedBody) {
                break;
            }
        } else if (request.size() > 16384) {
            break;
        }
    }

    if (request.empty() || headerEndPos == std::string::npos) {
        return;
    }

    std::string header = request.substr(0, headerEndPos);
    std::string body;
    if (expectedBody > 0 && request.size() >= headerEndPos + 4) {
        body = request.substr(headerEndPos + 4, expectedBody);
    }

    std::string firstLine;
    std::istringstream headerStream(header);
    std::getline(headerStream, firstLine);
    if (!firstLine.empty() && firstLine.back() == '\r') {
        firstLine.pop_back();
    }

    if (firstLine.rfind("GET /", 0) == 0) {
        sendHttpResponse(clientFd, "200 OK", "text/html", buildHtmlPage());
        return;
    }

    if (firstLine.rfind("POST /execute", 0) == 0) {
        std::string result = engine_.executeStatementWeb(body);
        sendHttpResponse(clientFd, "200 OK", "text/plain", result);
        return;
    }

    sendHttpResponse(clientFd, "404 Not Found", "text/plain", "Not Found");
}

void HttpServer::sendHttpResponse(int clientFd, const std::string &status, const std::string &contentType, const std::string &body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << body;
    std::string data = response.str();
    send(clientFd, data.c_str(), data.size(), 0);
}

std::string HttpServer::buildHtmlPage() const {
    static const char *html = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<title>Minimal SQL Interpreter</title>
<style>
body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, #1d2671, #c33764);
    color: #fff;
    min-height: 100vh;
    margin: 0;
    display: flex;
    align-items: center;
    justify-content: center;
}
.container {
    background: rgba(0, 0, 0, 0.45);
    border-radius: 16px;
    padding: 32px;
    width: 90%;
    max-width: 900px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.4);
}
textarea {
    width: 100%;
    min-height: 180px;
    border-radius: 12px;
    border: none;
    padding: 16px;
    font-size: 1rem;
    font-family: 'Fira Code', monospace;
    resize: vertical;
}
button {
    background: #ff6b6b;
    border: none;
    padding: 12px 28px;
    border-radius: 30px;
    font-size: 1rem;
    color: #fff;
    cursor: pointer;
    transition: transform 0.2s ease, box-shadow 0.2s ease;
}
button:hover {
    transform: translateY(-2px);
    box-shadow: 0 6px 20px rgba(0,0,0,0.25);
}
pre {
    background: rgba(255, 255, 255, 0.1);
    padding: 16px;
    border-radius: 12px;
    overflow-x: auto;
}
label {
    display: block;
    margin-bottom: 8px;
    font-weight: 600;
}
</style>
</head>
<body>
<div class="container">
    <h1>Minimal SQL Interpreter</h1>
    <p>Execute CREATE, INSERT, and SELECT statements directly from your browser. Use Ctrl+Enter to run queries.</p>
    <label for="sqlInput">SQL Command</label>
    <textarea id="sqlInput" placeholder="Example:\nCREATE TABLE users (id, name);\nINSERT INTO users VALUES (1, Alice);\nSELECT * FROM users;"></textarea>
    <div style="margin-top: 16px;">
        <button onclick="executeSql()">Run Query</button>
    </div>
    <h2>Result</h2>
    <pre id="result">Results appear here...</pre>
</div>
<script>
async function executeSql() {
    const input = document.getElementById('sqlInput');
    const result = document.getElementById('result');
    result.textContent = 'Running...';
    try {
        const response = await fetch('/execute', {
            method: 'POST',
            headers: { 'Content-Type': 'text/plain' },
            body: input.value
        });
        result.textContent = await response.text();
    } catch (err) {
        result.textContent = 'Error: ' + err.message;
    }
}

document.getElementById('sqlInput').addEventListener('keydown', function(e) {
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
        e.preventDefault();
        executeSql();
    }
});
</script>
</body>
</html>)HTML";
    return html;
}
