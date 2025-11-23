#include "HttpServer.h"
#include "Utils.h"
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

void HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error: Failed to create socket\n";
        return;
    }
    
    // Allow socket reuse
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to port
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Failed to bind to port " << port_ << "\n";
        close(serverSocket_);
        return;
    }
    
    // Listen for connections
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Error: Failed to listen on socket\n";
        close(serverSocket_);
        return;
    }
    
    running_ = true;
    std::cout << "HTTP Server running on http://localhost:" << port_ << "\n";
    std::cout << "Press Ctrl+C to stop\n";
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error: Failed to accept connection\n";
            }
            continue;
        }
        
        handleClient(clientSocket);
    }
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
        close(clientSocket);
        return;
    }
    
    buffer[bytesRead] = '\0';
    std::string request(buffer);
    
    std::string response;
    
    // Parse request
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;
    
    if (method == "GET" && path == "/") {
        // Serve main page
        std::string html = getIndexHtml();
        response = buildResponse(html, "text/html");
    }
    else if (method == "POST" && path == "/execute") {
        // Find request body
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            std::string body = request.substr(bodyPos + 4);
            
            // Parse form data (sql=...)
            std::string sql;
            if (body.substr(0, 4) == "sql=") {
                sql = urlDecode(body.substr(4));
            }
            
            // Execute SQL
            std::string result = engine_->executeStatementWeb(sql);
            
            // Escape HTML
            std::string escaped;
            for (char c : result) {
                if (c == '<') escaped += "&lt;";
                else if (c == '>') escaped += "&gt;";
                else if (c == '&') escaped += "&amp;";
                else if (c == '"') escaped += "&quot;";
                else escaped += c;
            }
            
            response = buildResponse(escaped, "text/plain");
        }
    }
    else {
        // 404 Not Found
        response = buildResponse("<h1>404 Not Found</h1>", "text/html");
    }
    
    send(clientSocket, response.c_str(), response.length(), 0);
    close(clientSocket);
}

std::string HttpServer::buildResponse(const std::string& content, const std::string& contentType) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    oss << "Content-Length: " << content.length() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << content;
    return oss.str();
}

std::string HttpServer::getIndexHtml() const {
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
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0, 0, 0, 0.2);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        .header p {
            opacity: 0.9;
            font-size: 1.1em;
        }
        
        .content {
            padding: 30px;
        }
        
        .section {
            margin-bottom: 25px;
        }
        
        .section h2 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.3em;
        }
        
        .examples {
            background: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            border-left: 4px solid #667eea;
            margin-bottom: 20px;
        }
        
        .examples code {
            display: block;
            margin: 5px 0;
            color: #333;
            font-family: 'Courier New', monospace;
        }
        
        textarea {
            width: 100%;
            min-height: 120px;
            padding: 15px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 14px;
            resize: vertical;
            transition: border-color 0.3s;
        }
        
        textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .button-container {
            margin-top: 15px;
            display: flex;
            gap: 10px;
            align-items: center;
        }
        
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 8px;
            font-size: 16px;
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
        
        .shortcut-hint {
            color: #666;
            font-size: 14px;
        }
        
        #result {
            background: #f8f9fa;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            padding: 20px;
            min-height: 150px;
            font-family: 'Courier New', monospace;
            white-space: pre-wrap;
            overflow-x: auto;
        }
        
        .error {
            color: #dc3545;
            font-weight: bold;
        }
        
        .success {
            color: #28a745;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🗄️ MiniSQL Web Interface</h1>
            <p>Execute SQL commands in your browser</p>
        </div>
        
        <div class="content">
            <div class="section">
                <h2>Example SQL Commands</h2>
                <div class="examples">
                    <code>CREATE TABLE users (id, name, age);</code>
                    <code>INSERT INTO users VALUES (1, Alice, 30);</code>
                    <code>SELECT * FROM users;</code>
                    <code>SELECT * FROM users WHERE age = 30;</code>
                </div>
            </div>
            
            <div class="section">
                <h2>SQL Command</h2>
                <textarea id="sqlInput" placeholder="Enter your SQL command here..."></textarea>
                <div class="button-container">
                    <button onclick="executeSql()">Execute</button>
                    <span class="shortcut-hint">💡 Tip: Press Ctrl+Enter to execute</span>
                </div>
            </div>
            
            <div class="section">
                <h2>Result</h2>
                <div id="result">Ready to execute SQL commands...</div>
            </div>
        </div>
    </div>
    
    <script>
        function executeSql() {
            const sql = document.getElementById('sqlInput').value.trim();
            const resultDiv = document.getElementById('result');
            
            if (!sql) {
                resultDiv.innerHTML = '<span class="error">Please enter a SQL command</span>';
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
            .then(data => {
                if (data.startsWith('Error:')) {
                    resultDiv.innerHTML = '<span class="error">' + data + '</span>';
                } else if (data === 'OK') {
                    resultDiv.innerHTML = '<span class="success">✓ ' + data + '</span>';
                } else {
                    resultDiv.textContent = data;
                }
            })
            .catch(error => {
                resultDiv.innerHTML = '<span class="error">Network error: ' + error + '</span>';
            });
        }
        
        // Ctrl+Enter shortcut
        document.getElementById('sqlInput').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                e.preventDefault();
                executeSql();
            }
        });
    </script>
</body>
</html>)HTML";
}

std::string HttpServer::urlDecode(const std::string& str) const {
    std::string result;
    char ch;
    int i, ii;
    
    for (i = 0; i < str.length(); i++) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%') {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            result += ch;
            i += 2;
        } else {
            result += str[i];
        }
    }
    
    return result;
}
