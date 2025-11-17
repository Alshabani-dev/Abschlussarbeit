#include "HttpServer.h"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <cstring>

HttpServer::HttpServer(int port, std::function<std::string(const std::string&)> sqlExecutor)
    : port_(port), sqlExecutor_(sqlExecutor), running_(false) {}

void HttpServer::start() {
    running_ = true;

    // Create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        close(serverSocket);
        return;
    }

    // Bind socket
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(serverSocket);
        return;
    }

    // Listen for connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "Server started on port " << port_ << std::endl;

    // Accept connections in a loop
    while (running_) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error accepting connection" << std::endl;
            }
            continue;
        }

        // Handle client in a separate thread
        std::thread clientThread(&HttpServer::handleClient, this, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
}

void HttpServer::stop() {
    running_ = false;
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[1024] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

    if (bytesRead < 0) {
        std::cerr << "Error reading from socket" << std::endl;
        close(clientSocket);
        return;
    }

    // Parse HTTP request
    std::string request(buffer, bytesRead);
    size_t methodEnd = request.find(' ');
    if (methodEnd == std::string::npos) {
        sendResponse(clientSocket, 400, "Bad Request");
        close(clientSocket);
        return;
    }

    size_t pathEnd = request.find(' ', methodEnd + 1);
    if (pathEnd == std::string::npos) {
        sendResponse(clientSocket, 400, "Bad Request");
        close(clientSocket);
        return;
    }

    std::string path = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);

    // Handle GET request
    if (request.substr(0, methodEnd) == "GET") {
        if (path == "/") {
            // Serve the main HTML page
            std::string html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Minimal SQL Interpreter</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        #sql-input {
            width: 100%;
            height: 100px;
            margin-bottom: 10px;
            padding: 10px;
            font-family: monospace;
        }
        #execute-btn {
            padding: 10px 20px;
            background-color: #4CAF50;
            color: white;
            border: none;
            cursor: pointer;
            font-size: 16px;
        }
        #execute-btn:hover {
            background-color: #45a049;
        }
        #result {
            margin-top: 20px;
            padding: 15px;
            background-color: #fff;
            border-radius: 5px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
            white-space: pre;
            font-family: monospace;
        }
        .examples {
            margin-top: 20px;
            padding: 10px;
            background-color: #f8f9fa;
            border-radius: 5px;
        }
    </style>
</head>
<body>
    <h1>Minimal SQL Interpreter</h1>
    <div>
        <textarea id="sql-input" placeholder="Enter SQL command here..."></textarea>
    </div>
    <div>
        <button id="execute-btn" onclick="executeSql()">Execute</button>
    </div>
    <div id="result"></div>

    <div class="examples">
        <h3>Examples:</h3>
        <ul>
            <li>CREATE TABLE users (id, name, age);</li>
            <li>INSERT INTO users VALUES (1, "Alice", 30);</li>
            <li>SELECT * FROM users;</li>
            <li>SELECT * FROM users WHERE age = 30;</li>
        </ul>
    </div>

    <script>
        function executeSql() {
            const sql = document.getElementById('sql-input').value;
            const resultDiv = document.getElementById('result');

            if (!sql.trim()) {
                resultDiv.textContent = 'Please enter an SQL command.';
                return;
            }

            resultDiv.textContent = 'Executing...';

            fetch('/execute', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'sql=' + encodeURIComponent(sql)
            })
            .then(response => response.text())
            .then(text => {
                resultDiv.textContent = text;
            })
            .catch(error => {
                resultDiv.textContent = 'Error: ' + error;
            });
        }

        // Allow Ctrl+Enter to execute
        document.getElementById('sql-input').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeSql();
            }
        });
    </script>
</body>
</html>
)HTML";

            sendResponse(clientSocket, 200, "text/html", html);
        } else if (path == "/execute" && request.find("POST") != std::string::npos) {
            // Handle SQL execution
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart == std::string::npos) {
                sendResponse(clientSocket, 400, "Bad Request");
                close(clientSocket);
                return;
            }

            std::string body = request.substr(bodyStart + 4);
            size_t sqlStart = body.find("sql=");
            if (sqlStart == std::string::npos) {
                sendResponse(clientSocket, 400, "Bad Request");
                close(clientSocket);
                return;
            }

            std::string sql = body.substr(sqlStart + 4);
            // Remove URL encoding
            size_t plusPos;
            while ((plusPos = sql.find('+')) != std::string::npos) {
                sql.replace(plusPos, 1, " ");
            }

            // Execute SQL
            std::string result = sqlExecutor_(sql);

            sendResponse(clientSocket, 200, "text/plain", result);
        } else {
            sendResponse(clientSocket, 404, "Not Found");
        }
    } else {
        sendResponse(clientSocket, 405, "Method Not Allowed");
    }

    close(clientSocket);
}

void HttpServer::sendResponse(int clientSocket, int statusCode, const std::string& statusText, const std::string& content) {
    std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += content;

    send(clientSocket, response.c_str(), response.length(), 0);
}

void HttpServer::sendResponse(int clientSocket, int statusCode, const std::string& statusText) {
    std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n";
    response += "Content-Length: 0\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";

    send(clientSocket, response.c_str(), response.length(), 0);
}
