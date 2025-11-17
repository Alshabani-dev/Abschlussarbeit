#include "HttpServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <arpa/inet.h>

HttpServer::HttpServer(int port, const std::function<std::string(const std::string&)>& sqlExecutor)
    : port_(port), sqlExecutor_(sqlExecutor), serverSocket_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    // Create socket
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

    running_ = true;
    std::cout << "Server listening on port " << port_ << std::endl;

    // Accept connections
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

        handleClient(clientSocket);
        close(clientSocket);
    }
}

void HttpServer::stop() {
    if (running_) {
        running_ = false;
        if (serverSocket_ >= 0) {
            close(serverSocket_);
            serverSocket_ = -1;
        }
    }
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[1024] = {0};
    read(clientSocket, buffer, 1024);

    // Parse HTTP request
    std::string request(buffer);
    size_t methodEnd = request.find(' ');
    if (methodEnd == std::string::npos) {
        send(clientSocket, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
        return;
    }

    size_t pathEnd = request.find(' ', methodEnd + 1);
    if (pathEnd == std::string::npos) {
        send(clientSocket, "HTTP/1.1 400 Bad Request\r\n\r\n", 29, 0);
        return;
    }

    std::string path = request.substr(methodEnd + 1, pathEnd - methodEnd - 1);

    // Handle GET request
    if (request.substr(0, methodEnd) == "GET") {
        if (path == "/") {
            // Send HTML form
            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            response += getHtmlTemplate();
            send(clientSocket, response.c_str(), response.size(), 0);
        } else if (path.find("/execute?sql=") == 0) {
            // Execute SQL and return results
            std::string sql = path.substr(13); // Skip "/execute?sql="
            // URL decode the SQL
            std::string decodedSql;
            for (size_t i = 0; i < sql.size(); i++) {
                if (sql[i] == '%' && i + 2 < sql.size()) {
                    int hexValue;
                    std::string hex = sql.substr(i+1, 2);
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

            // Escape newlines for HTML
            std::string escapedResult;
            for (char c : result) {
                if (c == '\n') {
                    escapedResult += "<br>";
                } else {
                    escapedResult += c;
                }
            }

            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            response += R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>SQL Execution Result</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        pre { background: #f5f5f5; padding: 10px; border-radius: 5px; }
        a { display: inline-block; margin-top: 10px; color: #0066cc; text-decoration: none; }
    </style>
</head>
<body>
    <h1>SQL Execution Result</h1>
    <pre>)HTML" + escapedResult + R"HTML(</pre>
    <a href="/">Back to SQL Input</a>
</body>
</html>
)HTML";
            send(clientSocket, response.c_str(), response.size(), 0);
        } else {
            send(clientSocket, "HTTP/1.1 404 Not Found\r\n\r\n", 26, 0);
        }
    } else {
        send(clientSocket, "HTTP/1.1 405 Method Not Allowed\r\n\r\n", 32, 0);
    }
}

std::string HttpServer::getHtmlTemplate() {
    return R"HTML(
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
        .sql-form {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
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
            background: #3498db;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover {
            background: #2980b9;
        }
        .examples {
            margin-top: 20px;
            background: white;
            padding: 15px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .examples h3 {
            margin-top: 0;
            color: #2c3e50;
        }
        .examples pre {
            background: #f8f9fa;
            padding: 10px;
            border-radius: 4px;
            overflow-x: auto;
        }
    </style>
</head>
<body>
    <h1>Minimal SQL Interpreter</h1>

    <div class="sql-form">
        <form id="sqlForm">
            <textarea id="sqlInput" placeholder="Enter your SQL command here..."></textarea>
            <br>
            <button type="button" onclick="executeSql()">Execute SQL</button>
        </form>
    </div>

    <div class="examples">
        <h3>Example Commands:</h3>
        <pre>CREATE TABLE users (id, name, age);</pre>
        <pre>INSERT INTO users VALUES (1, "Alice", 30);</pre>
        <pre>SELECT * FROM users;</pre>
        <pre>SELECT * FROM users WHERE age = 30;</pre>
    </div>

    <script>
        function executeSql() {
            const sql = document.getElementById('sqlInput').value;
            if (sql.trim() === '') {
                alert('Please enter an SQL command');
                return;
            }

            // Simple URL encoding for the SQL
            const encodedSql = encodeURIComponent(sql);
            window.location.href = `/execute?sql=${encodedSql}`;
        }

        // Allow Ctrl+Enter to execute
        document.getElementById('sqlInput').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                e.preventDefault();
                executeSql();
            }
        });
    </script>
</body>
</html>
)HTML";
}
