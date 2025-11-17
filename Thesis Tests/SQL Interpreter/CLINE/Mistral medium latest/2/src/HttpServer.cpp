#include "HttpServer.h"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>

HttpServer::HttpServer(int port, std::function<std::string(const std::string&)> sqlExecutor)
    : port_(port), sqlExecutor_(sqlExecutor) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    running_ = true;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_len);

        if (client_socket < 0) {
            if (running_) {
                std::cerr << "Error accepting connection" << std::endl;
            }
            continue;
        }

        // Handle client in a separate thread
        std::thread([this, client_socket]() {
            handleClient(client_socket);
            close(client_socket);
        }).detach();
    }

    close(server_fd);
}

void HttpServer::stop() {
    running_ = false;
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[1024] = {0};
    read(clientSocket, buffer, 1024);

    std::string request(buffer);
    std::string response;

    // Simple HTTP request parsing
    if (request.find("GET / ") != std::string::npos ||
        request.find("GET /index.html ") != std::string::npos) {
        response = generateResponse("");
    } else if (request.find("POST /execute ") != std::string::npos) {
        // Extract SQL from POST data
        size_t sqlStart = request.find("sql=");
        if (sqlStart != std::string::npos) {
            sqlStart += 4; // Skip "sql="
            size_t sqlEnd = request.find(" HTTP/", sqlStart);
            if (sqlEnd == std::string::npos) {
                sqlEnd = request.find("&", sqlStart);
                if (sqlEnd == std::string::npos) {
                    sqlEnd = request.length();
                }
            }

            std::string sql = request.substr(sqlStart, sqlEnd - sqlStart);
            // URL decode the SQL
            std::string decodedSql;
            for (size_t i = 0; i < sql.length(); i++) {
                if (sql[i] == '+') {
                    decodedSql += ' ';
                } else if (sql[i] == '%' && i + 2 < sql.length()) {
                    unsigned int hexValue;
                    sscanf(sql.substr(i + 1, 2).c_str(), "%x", &hexValue);
                    decodedSql += static_cast<char>(hexValue);
                    i += 2;
                } else {
                    decodedSql += sql[i];
                }
            }

            response = generateResponse(decodedSql);
        } else {
            response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMissing SQL parameter";
        }
    } else {
        response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nPage not found";
    }

    send(clientSocket, response.c_str(), response.size(), 0);
}

std::string HttpServer::generateResponse(const std::string& sql) {
    std::string html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Minimal SQL Interpreter</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background: linear-gradient(135deg, #6e8efb, #a777e3);
            min-height: 100vh;
            color: white;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: rgba(0, 0, 0, 0.5);
            border-radius: 10px;
            box-shadow: 0 0 20px rgba(0, 0, 0, 0.3);
        }
        h1 {
            text-align: center;
            color: #fff;
        }
        textarea {
            width: 100%;
            height: 100px;
            padding: 10px;
            border-radius: 5px;
            border: none;
            margin-bottom: 10px;
            font-family: monospace;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
        }
        button:hover {
            background-color: #45a049;
        }
        #output {
            background-color: rgba(0, 0, 0, 0.7);
            padding: 15px;
            border-radius: 5px;
            white-space: pre-wrap;
            font-family: monospace;
            min-height: 200px;
            margin-top: 20px;
        }
        .examples {
            background-color: rgba(0, 0, 0, 0.3);
            padding: 15px;
            border-radius: 5px;
            margin-top: 20px;
        }
        .examples h3 {
            margin-top: 0;
        }
        .example {
            background-color: rgba(255, 255, 255, 0.1);
            padding: 10px;
            margin: 5px 0;
            border-radius: 3px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Minimal SQL Interpreter</h1>

        <form id="sqlForm">
            <textarea id="sqlInput" placeholder="Enter SQL command..."></textarea>
            <button type="button" onclick="executeSQL()">Execute</button>
        </form>

        <div id="output"></div>

        <div class="examples">
            <h3>Example Commands:</h3>
            <div class="example" onclick="setExample('CREATE TABLE users (id, name, age);')">
                CREATE TABLE users (id, name, age);
            </div>
            <div class="example" onclick="setExample('INSERT INTO users VALUES (1, Alice, 30);')">
                INSERT INTO users VALUES (1, Alice, 30);
            </div>
            <div class="example" onclick="setExample('SELECT * FROM users;')">
                SELECT * FROM users;
            </div>
            <div class="example" onclick="setExample('SELECT * FROM users WHERE age = 30;')">
                SELECT * FROM users WHERE age = 30;
            </div>
        </div>
    </div>

    <script>
        function executeSQL() {
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
                output.innerHTML = data.replace(/\n/g, '<br>');
            })
            .catch(error => {
                output.innerHTML = 'Error: ' + error;
            });
        }

        function setExample(example) {
            document.getElementById('sqlInput').value = example;
        }

        // Allow Ctrl+Enter to execute
        document.getElementById('sqlInput').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeSQL();
            }
        });
    </script>
</body>
</html>
)HTML";

    if (!sql.empty()) {
        std::string result = sqlExecutor_(sql);
        // Replace newlines with <br> for HTML display
        std::string htmlResult;
        for (char c : result) {
            if (c == '\n') {
                htmlResult += "<br>";
            } else {
                htmlResult += c;
            }
        }

        // Replace the output div with the result
        size_t outputPos = html.find("<div id=\"output\"></div>");
        if (outputPos != std::string::npos) {
            html.replace(outputPos, 25, "<div id=\"output\">" + htmlResult + "</div>");
        }
    }

    return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html;
}
