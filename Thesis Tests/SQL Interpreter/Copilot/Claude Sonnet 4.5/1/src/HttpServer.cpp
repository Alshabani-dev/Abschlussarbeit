#include "HttpServer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

HttpServer::HttpServer(Engine* engine, int port)
    : engine_(engine), port_(port), serverSocket_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error: Failed to create socket\n";
        return false;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: Failed to set socket options\n";
        close(serverSocket_);
        return false;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error: Failed to bind to port " << port_ << "\n";
        close(serverSocket_);
        return false;
    }
    
    // Listen
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Error: Failed to listen on socket\n";
        close(serverSocket_);
        return false;
    }
    
    running_ = true;
    std::cout << "HTTP Server started on port " << port_ << "\n";
    std::cout << "Open your browser to: http://localhost:" << port_ << "\n";
    std::cout << "Press Ctrl+C to stop the server\n\n";
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddress;
        socklen_t clientLen = sizeof(clientAddress);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error: Failed to accept connection\n";
            }
            continue;
        }
        
        handleClient(clientSocket);
        close(clientSocket);
    }
    
    return true;
}

void HttpServer::stop() {
    running_ = false;
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[4096];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
        return;
    }
    
    buffer[bytesRead] = '\0';
    std::string request(buffer);
    
    std::string method, path, body;
    parseHttpRequest(request, method, path, body);
    
    std::cout << "Request: " << method << " " << path << std::endl;
    
    std::string response;
    
    if (method == "GET" && path == "/") {
        // Serve the HTML page
        std::string html = getIndexHtml();
        response = createHttpResponse(200, "text/html; charset=utf-8", html);
        std::cout << "Serving index.html (" << html.length() << " bytes)" << std::endl;
    } else if (method == "POST" && path == "/execute") {
        // Execute SQL command
        size_t sqlPos = body.find("sql=");
        if (sqlPos != std::string::npos) {
            std::string sql = urlDecode(body.substr(sqlPos + 4));
            std::cout << "Executing SQL: " << sql << std::endl;
            std::string result = engine_->executeStatementWeb(sql);
            
            // Escape HTML entities in result
            std::string escapedResult;
            for (char c : result) {
                switch (c) {
                    case '<': escapedResult += "&lt;"; break;
                    case '>': escapedResult += "&gt;"; break;
                    case '&': escapedResult += "&amp;"; break;
                    default: escapedResult += c;
                }
            }
            
            response = createHttpResponse(200, "text/plain; charset=utf-8", escapedResult);
        } else {
            response = createHttpResponse(400, "text/plain", "Bad Request: Missing sql parameter");
        }
    } else {
        response = createHttpResponse(404, "text/plain", "Not Found");
    }
    
    ssize_t sent = send(clientSocket, response.c_str(), response.length(), 0);
    std::cout << "Sent " << sent << " bytes" << std::endl;
}

std::string HttpServer::parseHttpRequest(const std::string& request, std::string& method, std::string& path, std::string& body) {
    std::istringstream stream(request);
    std::string line;
    
    // Parse first line (method and path)
    if (std::getline(stream, line)) {
        size_t firstSpace = line.find(' ');
        size_t secondSpace = line.find(' ', firstSpace + 1);
        
        if (firstSpace != std::string::npos && secondSpace != std::string::npos) {
            method = line.substr(0, firstSpace);
            path = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        }
    }
    
    // Skip headers
    std::string headers;
    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        headers += line + "\n";
    }
    
    // Get body
    body = "";
    while (std::getline(stream, line)) {
        body += line;
    }
    
    return headers;
}

std::string HttpServer::createHttpResponse(int statusCode, const std::string& contentType, const std::string& body) {
    std::ostringstream response;
    
    std::string statusText = (statusCode == 200) ? "OK" : "Not Found";
    
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    
    return response.str();
}

std::string HttpServer::getIndexHtml() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MiniSQL Web Interface</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
        }
        
        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            max-width: 900px;
            width: 100%;
            padding: 30px;
        }
        
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
        }
        
        .subtitle {
            color: #666;
            margin-bottom: 25px;
            font-size: 14px;
        }
        
        .examples {
            background: #f8f9fa;
            border-left: 4px solid #667eea;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 4px;
        }
        
        .examples h3 {
            color: #667eea;
            font-size: 14px;
            margin-bottom: 10px;
        }
        
        .examples code {
            display: block;
            background: white;
            padding: 8px;
            margin: 5px 0;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            color: #333;
        }
        
        textarea {
            width: 100%;
            height: 150px;
            padding: 15px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            font-family: 'Courier New', monospace;
            font-size: 14px;
            resize: vertical;
            transition: border-color 0.3s;
        }
        
        textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .button-group {
            margin-top: 15px;
            display: flex;
            gap: 10px;
        }
        
        button {
            padding: 12px 30px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
        }
        
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        button:active {
            transform: translateY(0);
        }
        
        .clear-btn {
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        }
        
        .result {
            margin-top: 20px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 6px;
            min-height: 100px;
            max-height: 400px;
            overflow: auto;
        }
        
        .result pre {
            font-family: 'Courier New', monospace;
            font-size: 13px;
            color: #333;
            white-space: pre;
            margin: 0;
        }
        
        .shortcut-hint {
            color: #999;
            font-size: 12px;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🗄️ MiniSQL Web Interface</h1>
        <p class="subtitle">Execute SQL commands in your browser</p>
        
        <div class="examples">
            <h3>Example Commands:</h3>
            <code>CREATE TABLE users (id, name, age);</code>
            <code>INSERT INTO users VALUES (1, Alice, 30);</code>
            <code>SELECT * FROM users;</code>
            <code>SELECT * FROM users WHERE name = Alice;</code>
        </div>
        
        <textarea id="sqlInput" placeholder="Enter your SQL command here..."></textarea>
        
        <div class="button-group">
            <button onclick="executeSql()">Execute</button>
            <button class="clear-btn" onclick="clearResult()">Clear</button>
        </div>
        
        <p class="shortcut-hint">💡 Tip: Press Ctrl+Enter to execute</p>
        
        <div class="result">
            <pre id="output">Ready. Enter a SQL command and click Execute.</pre>
        </div>
    </div>
    
    <script>
        const sqlInput = document.getElementById('sqlInput');
        const output = document.getElementById('output');
        
        // Ctrl+Enter to execute
        sqlInput.addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeSql();
            }
        });
        
        function executeSql() {
            const sql = sqlInput.value.trim();
            
            if (!sql) {
                output.textContent = 'Error: Please enter a SQL command.';
                return;
            }
            
            output.textContent = 'Executing...';
            
            fetch('/execute', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded'
                },
                body: 'sql=' + encodeURIComponent(sql)
            })
            .then(response => response.text())
            .then(result => {
                output.textContent = result;
            })
            .catch(error => {
                output.textContent = 'Error: ' + error.message;
            });
        }
        
        function clearResult() {
            sqlInput.value = '';
            output.textContent = 'Ready. Enter a SQL command and click Execute.';
            sqlInput.focus();
        }
    </script>
</body>
</html>)HTML";
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.length()) {
            std::string hex = str.substr(i + 1, 2);
            char c = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += c;
            i += 2;
        } else {
            result += str[i];
        }
    }
    return result;
}
