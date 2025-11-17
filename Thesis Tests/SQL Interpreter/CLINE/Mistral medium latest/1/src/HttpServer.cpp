#include "HttpServer.h"
#include "Engine.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <sstream>
#include <iostream>

HttpServer::HttpServer(int port) : port_(port) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;
    running_ = true;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        std::thread(&HttpServer::handleRequest, this, clientSocket).detach();
    }

    close(serverSocket);
}

void HttpServer::stop() {
    running_ = false;
}

void HttpServer::handleRequest(int clientSocket) {
    char buffer[1024] = {0};
    read(clientSocket, buffer, 1024);

    std::string request(buffer);
    std::string response;

    if (request.find("POST /execute") != std::string::npos) {
        // Extract SQL from POST data
        size_t sqlStart = request.find("sql=");
        if (sqlStart != std::string::npos) {
            sqlStart += 4;
            size_t sqlEnd = request.find("&", sqlStart);
            if (sqlEnd == std::string::npos) {
                sqlEnd = request.find(" ", sqlStart);
            }

            std::string sql = request.substr(sqlStart, sqlEnd - sqlStart);
            // URL decode the SQL
            std::string decoded;
            for (size_t i = 0; i < sql.length(); i++) {
                if (sql[i] == '+') {
                    decoded += ' ';
                } else if (sql[i] == '%' && i + 2 < sql.length()) {
                    // Convert hex to char
                    std::string hex = sql.substr(i + 1, 2);
                    char ch = (char)strtol(hex.c_str(), nullptr, 16);
                    decoded += ch;
                    i += 2;
                } else {
                    decoded += sql[i];
                }
            }
            sql = decoded;

            Engine engine;
            response = generateResponse(sql);
        } else {
            response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMissing SQL parameter";
        }
    } else {
        response = generateHtmlResponse();
    }

    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

std::string HttpServer::generateResponse(const std::string& sql) {
    Engine engine;
    std::string result = engine.executeStatementWeb(sql);

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/plain\r\n";
    response << "Content-Length: " << result.size() << "\r\n";
    response << "\r\n";
    response << result;

    return response.str();
}

std::string HttpServer::generateHtmlResponse() {
    std::ostringstream html;
    html << R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Minimal SQL Interpreter</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        textarea {
            width: 100%;
            height: 100px;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-family: monospace;
            margin-bottom: 10px;
        }
        button {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 10px 15px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover {
            background-color: #45a049;
        }
        #output {
            background-color: #f9f9f9;
            border: 1px solid #ddd;
            padding: 10px;
            border-radius: 4px;
            font-family: monospace;
            white-space: pre;
            margin-top: 10px;
            min-height: 100px;
        }
        .examples {
            margin-top: 20px;
            padding: 10px;
            background-color: #f0f8ff;
            border-radius: 4px;
        }
        .examples h3 {
            margin-top: 0;
        }
        .examples ul {
            padding-left: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Minimal SQL Interpreter</h1>
        <textarea id="sqlInput" placeholder="Enter SQL command here..."></textarea>
        <div>
            <button onclick="executeSql()">Execute</button>
        </div>
        <div id="output"></div>

        <div class="examples">
            <h3>Example Commands:</h3>
            <ul>
                <li>CREATE TABLE users (id, name, age);</li>
                <li>INSERT INTO users VALUES (1, "Alice", 30);</li>
                <li>SELECT * FROM users;</li>
                <li>SELECT * FROM users WHERE age = 30;</li>
            </ul>
        </div>
    </div>

    <script>
        function executeSql() {
            const sql = document.getElementById('sqlInput').value;
            const output = document.getElementById('output');

            fetch('/execute', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'sql=' + encodeURIComponent(sql)
            })
            .then(response => response.text())
            .then(data => {
                output.textContent = data;
            })
            .catch(error => {
                output.textContent = 'Error: ' + error;
            });
        }

        // Allow Ctrl+Enter to execute
        document.getElementById('sqlInput').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeSql();
            }
        });
    </script>
</body>
</html>
)HTML";

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << html.str().size() << "\r\n";
    response << "\r\n";
    response << html.str();

    return response.str();
}
