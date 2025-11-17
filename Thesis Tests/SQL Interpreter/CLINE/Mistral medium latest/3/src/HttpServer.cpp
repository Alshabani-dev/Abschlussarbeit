#include "HttpServer.h"
#include "Utils.h"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <map>
#include <sstream>

HttpServer::HttpServer(int port, Engine &engine) : port_(port), running_(false), serverSocket_(-1), engine_(engine) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_) {
        return;
    }

    running_ = true;

    // Create server socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        close(serverSocket_);
        return;
    }

    // Bind socket
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(serverSocket_);
        return;
    }

    // Listen
    if (listen(serverSocket_, 5) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket_);
        return;
    }

    std::cout << "HTTP server listening on port " << port_ << std::endl;

    // Accept connections in a loop
    while (running_) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error accepting connection" << std::endl;
            }
            continue;
        }

        // Handle client in a separate thread
        std::thread(&HttpServer::handleClient, this, clientSocket).detach();
    }
}

void HttpServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[1024] = {0};
    read(clientSocket, buffer, 1024);

    std::string request(buffer);
    std::istringstream requestStream(request);

    // Parse request line
    std::string method, path, version;
    requestStream >> method >> path >> version;

    // Parse headers
    std::string header;
    std::map<std::string, std::string> headers;
    while (std::getline(requestStream, header) && header != "\r") {
        size_t colonPos = header.find(':');
        if (colonPos != std::string::npos) {
            std::string key = header.substr(0, colonPos);
            std::string value = header.substr(colonPos + 1);
            headers[Utils::trim(key)] = Utils::trim(value);
        }
    }

    // Parse body if POST
    std::string body;
    if (method == "POST") {
        size_t contentLength = 0;
        if (headers.find("Content-Length") != headers.end()) {
            contentLength = std::stoul(headers["Content-Length"]);
        }

        if (contentLength > 0) {
            std::vector<char> bodyBuffer(contentLength + 1, 0);
            read(clientSocket, bodyBuffer.data(), contentLength);
            body = std::string(bodyBuffer.data());
        }
    }

    // Handle routes
    std::string response;
    if (path == "/") {
        response = buildResponse(R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Minimal SQL Interpreter</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        textarea { width: 100%; height: 100px; }
        button { padding: 8px 16px; }
        table { border-collapse: collapse; margin-top: 20px; }
        th, td { border: 1px solid #ddd; padding: 8px; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <h1>Minimal SQL Interpreter</h1>
    <form id="sqlForm">
        <textarea id="sqlInput" placeholder="Enter SQL command..."></textarea>
        <button type="submit">Execute</button>
    </form>
    <div id="result"></div>
    <script>
        document.getElementById('sqlForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const sql = document.getElementById('sqlInput').value;
            const response = await fetch('/execute', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'sql=' + encodeURIComponent(sql)
            });
            const result = await response.text();
            document.getElementById('result').innerHTML = '<pre>' + result + '</pre>';
        });
    </script>
</body>
</html>
)HTML", "text/html");
    } else if (path == "/execute" && method == "POST") {
        // Extract SQL from body
        size_t sqlPos = body.find("sql=");
        if (sqlPos != std::string::npos) {
            std::string sql = body.substr(sqlPos + 4);
            response = buildResponse(engine_.executeStatementWeb(sql), "text/plain");
        } else {
            response = buildResponse("Error: No SQL command provided", "text/plain");
        }
    } else {
        response = buildResponse("404 Not Found", "text/plain", 404);
    }

    // Send response
    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

std::string HttpServer::buildResponse(const std::string &content, const std::string &contentType, int statusCode) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode << " OK\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << content.length() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << content;
    return oss.str();
}
