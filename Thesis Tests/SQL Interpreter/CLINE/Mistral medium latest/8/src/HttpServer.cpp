#include "HttpServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <arpa/inet.h>
#include <thread>
#include <iostream>

HttpServer::HttpServer(int port, Engine& engine) : port_(port), engine_(engine) {}

void HttpServer::start() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket creation failed");
        return;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server_fd);
        return;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        // Handle each client in a separate thread
        std::thread([this, client_socket]() {
            handleRequest(client_socket);
            close(client_socket);
        }).detach();
    }

    close(server_fd);
}

void HttpServer::handleRequest(int clientSocket) {
    char buffer[1024] = {0};
    read(clientSocket, buffer, 1024);

    // Parse HTTP request
    std::istringstream request(buffer);
    std::string method, path, version;
    request >> method >> path >> version;

    // Simple routing
    std::string response;
    if (path == "/") {
        response = R"HTML(
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
            color: #2c3e50;
        }
        textarea {
            width: 100%;
            height: 100px;
            margin: 10px 0;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        button {
            background-color: #3498db;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover {
            background-color: #2980b9;
        }
        #result {
            margin-top: 20px;
            padding: 15px;
            background-color: #f8f9fa;
            border-radius: 4px;
            white-space: pre-wrap;
        }
        .examples {
            margin: 20px 0;
            padding: 15px;
            background-color: #e8f4fc;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <h1>Minimal SQL Interpreter</h1>

    <div class="examples">
        <h3>Example Commands:</h3>
        <pre>
CREATE TABLE users (id, name, age);
INSERT INTO users VALUES (1, Alice, 30);
INSERT INTO users VALUES (2, Bob, 25);
SELECT * FROM users;
SELECT * FROM users WHERE age = 25;
        </pre>
    </div>

    <textarea id="sqlCommand" placeholder="Enter SQL command here..."></textarea>
    <br>
    <button onclick="executeCommand()">Execute</button>

    <div id="result"></div>

    <script>
        function executeCommand() {
            const command = document.getElementById('sqlCommand').value;
            const resultDiv = document.getElementById('result');

            if (!command.trim()) {
                resultDiv.innerHTML = 'Please enter a command';
                return;
            }

            resultDiv.innerHTML = 'Executing...';

            fetch('/execute', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'sql=' + encodeURIComponent(command)
            })
            .then(response => response.text())
            .then(data => {
                resultDiv.innerHTML = data.replace(/\n/g, '<br>');
            })
            .catch(error => {
                resultDiv.innerHTML = 'Error: ' + error;
            });
        }

        // Allow Ctrl+Enter to execute
        document.getElementById('sqlCommand').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeCommand();
            }
        });
    </script>
</body>
</html>
)HTML";
    } else if (path == "/execute" && method == "POST") {
        // Find the SQL parameter in the POST data
        std::string sql;
        size_t sqlPos = std::string(buffer).find("sql=");
        if (sqlPos != std::string::npos) {
            sql = std::string(buffer).substr(sqlPos + 4);
            // Remove URL encoding and any trailing data
            size_t endPos = sql.find(' ');
            if (endPos != std::string::npos) {
                sql = sql.substr(0, endPos);
            }
            // Simple URL decoding for spaces
            size_t plusPos;
            while ((plusPos = sql.find('+')) != std::string::npos) {
                sql.replace(plusPos, 1, " ");
            }
        }

        response = generateResponse(sql);
    } else {
        response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
        send(clientSocket, response.c_str(), response.size(), 0);
        return;
    }

    // Send HTTP response
    std::string httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " +
                              std::to_string(response.size()) + "\r\n\r\n" + response;
    send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0);
}

std::string HttpServer::generateResponse(const std::string& sql) {
    if (sql.empty()) {
        return "<div style='color: red;'>No SQL command provided</div>";
    }

    try {
        std::string result = engine_.executeStatementWeb(sql);
        return "<pre>" + result + "</pre>";
    } catch (const std::exception& e) {
        return "<div style='color: red;'>Error: " + std::string(e.what()) + "</div>";
    }
}
