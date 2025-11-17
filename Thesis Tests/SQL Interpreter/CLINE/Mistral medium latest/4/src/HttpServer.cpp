#include "HttpServer.h"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <sstream>

HttpServer::HttpServer(int port, const std::function<std::string(const std::string&)>& sqlExecutor)
    : port_(port), sqlExecutor_(sqlExecutor), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    running_ = true;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return;
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return;
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
        // Accept incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        // Handle client in a separate thread
        std::thread clientThread(&HttpServer::handleClient, this, new_socket);
        clientThread.detach();
    }

    close(server_fd);
}

void HttpServer::stop() {
    running_ = false;
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[30000] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

    if (bytesRead < 0) {
        perror("read");
        close(clientSocket);
        return;
    }

    std::string request(buffer, bytesRead);
    std::string response = buildResponse(request);

    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

std::string HttpServer::buildResponse(const std::string& request) {
    // Check if this is a GET request with SQL command
    if (request.find("GET /execute?sql=") != std::string::npos) {
        size_t sqlStart = request.find("sql=") + 4; // Skip "sql="
        size_t sqlEnd = request.find(" ", sqlStart);

        if (sqlEnd == std::string::npos) {
            sqlEnd = request.find("HTTP/", sqlStart);
        }

        if (sqlStart < sqlEnd) {
            std::string sql = request.substr(sqlStart, sqlEnd - sqlStart);
            // URL decode the SQL
            std::string decodedSql;
            for (size_t i = 0; i < sql.size(); i++) {
                if (sql[i] == '%' && i + 2 < sql.size()) {
                    int hexValue;
                    std::string hex = sql.substr(i + 1, 2);
                    sscanf(hex.c_str(), "%x", &hexValue);
                    decodedSql += static_cast<char>(hexValue);
                    i += 2;
                } else if (sql[i] == '+') {
                    decodedSql += ' ';
                } else {
                    decodedSql += sql[i];
                }
            }

            std::string result = sqlExecutor_(decodedSql);

            // Escape special characters for JSON
            std::string escapedResult;
            for (char c : result) {
                switch (c) {
                    case '"': escapedResult += "\\\""; break;
                    case '\\': escapedResult += "\\\\"; break;
                    case '\b': escapedResult += "\\b"; break;
                    case '\f': escapedResult += "\\f"; break;
                    case '\n': escapedResult += "\\n"; break;
                    case '\r': escapedResult += "\\r"; break;
                    case '\t': escapedResult += "\\t"; break;
                    default:
                        if (c < 0x20) {
                            // Escape other control characters
                            char buf[7];
                            snprintf(buf, sizeof(buf), "\\u%04x", c);
                            escapedResult += buf;
                        } else {
                            escapedResult += c;
                        }
                }
            }

            // Build JSON response
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: application/json\r\n";
            response << "Access-Control-Allow-Origin: *\r\n";
            response << "Connection: close\r\n";
            response << "\r\n";
            response << "{\"result\": \"" << escapedResult << "\"}";

            return response.str();
        }
    }

    // Default response - HTML page
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << R"HTML(
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
            text-align: center;
        }
        #sql-input {
            width: 100%;
            height: 100px;
            padding: 10px;
            font-size: 16px;
            border: 1px solid #ddd;
            border-radius: 4px;
            margin-bottom: 10px;
        }
        button {
            background-color: #3498db;
            color: white;
            border: none;
            padding: 10px 20px;
            font-size: 16px;
            cursor: pointer;
            border-radius: 4px;
            width: 100%;
        }
        button:hover {
            background-color: #2980b9;
        }
        #result {
            margin-top: 20px;
            padding: 15px;
            background-color: #fff;
            border: 1px solid #ddd;
            border-radius: 4px;
            white-space: pre-wrap;
            min-height: 100px;
        }
        .examples {
            margin-top: 20px;
            padding: 15px;
            background-color: #f8f9fa;
            border-radius: 4px;
        }
        .examples h3 {
            margin-top: 0;
            color: #2c3e50;
        }
        .examples ul {
            padding-left: 20px;
        }
    </style>
</head>
<body>
    <h1>Minimal SQL Interpreter</h1>

    <div>
        <textarea id="sql-input" placeholder="Enter SQL command here..."></textarea>
        <button onclick="executeSql()">Execute</button>
    </div>

    <div id="result"></div>

    <div class="examples">
        <h3>Example Commands:</h3>
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

            resultDiv.textContent = "Executing...";

            fetch('/execute?sql=' + encodeURIComponent(sql))
                .then(response => response.json())
                .then(data => {
                    resultDiv.textContent = data.result;
                })
                .catch(error => {
                    resultDiv.textContent = "Error: " + error;
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

    return response.str();
}
