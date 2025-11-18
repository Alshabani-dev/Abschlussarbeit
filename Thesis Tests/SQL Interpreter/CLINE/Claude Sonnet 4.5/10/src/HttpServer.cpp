#include "HttpServer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

HttpServer::HttpServer(int port, Engine* engine) 
    : port_(port), serverSocket_(-1), running_(false), engine_(engine) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        return;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error: Failed to bind socket to port " << port_ << std::endl;
        close(serverSocket_);
        return;
    }
    
    // Listen
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Error: Failed to listen on socket" << std::endl;
        close(serverSocket_);
        return;
    }
    
    running_ = true;
    std::cout << "HTTP Server started on http://localhost:" << port_ << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error: Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        handleClient(clientSocket);
        close(clientSocket);
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
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    
    if (bytesRead <= 0) {
        return;
    }
    
    buffer[bytesRead] = '\0';
    std::string request(buffer);
    
    // Parse request line
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;
    
    std::string response;
    
    if (method == "GET" && path == "/") {
        // Serve HTML page
        response = buildResponse(getHtmlPage(), "text/html");
    }
    else if (method == "POST" && path == "/execute") {
        // Extract SQL from POST body
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            std::string body = request.substr(bodyPos + 4);
            
            // Parse form data (sql=...)
            std::string sql;
            size_t sqlPos = body.find("sql=");
            if (sqlPos != std::string::npos) {
                size_t endPos = body.find("&", sqlPos);
                if (endPos == std::string::npos) {
                    endPos = body.length();
                }
                sql = body.substr(sqlPos + 4, endPos - sqlPos - 4);
                sql = urlDecode(sql);
            }
            
            // Execute SQL
            std::string result = engine_->executeStatementWeb(sql);
            
            // Return plain text response
            response = buildResponse(result, "text/plain");
        } else {
            response = buildResponse("Error: Invalid request", "text/plain");
        }
    }
    else {
        // 404 Not Found
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }
    
    write(clientSocket, response.c_str(), response.length());
}

std::string HttpServer::parseRequest(const std::string& request) {
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        return request.substr(bodyPos + 4);
    }
    return "";
}

std::string HttpServer::buildResponse(const std::string& body, const std::string& contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

std::string HttpServer::getHtmlPage() {
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
        }
        
        .container {
            max-width: 1000px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
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
            font-size: 1.1em;
            opacity: 0.9;
        }
        
        .content {
            padding: 30px;
        }
        
        .section {
            margin-bottom: 30px;
        }
        
        .section h2 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 1.5em;
        }
        
        textarea {
            width: 100%;
            height: 120px;
            padding: 15px;
            font-family: 'Courier New', monospace;
            font-size: 14px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            resize: vertical;
            transition: border-color 0.3s;
        }
        
        textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            font-size: 16px;
            font-weight: bold;
            border-radius: 8px;
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
        
        .result {
            background: #f5f5f5;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            padding: 20px;
            min-height: 200px;
            font-family: 'Courier New', monospace;
            font-size: 14px;
            white-space: pre-wrap;
            word-wrap: break-word;
            overflow-x: auto;
        }
        
        .examples {
            background: #f9f9f9;
            border-left: 4px solid #667eea;
            padding: 15px;
            border-radius: 5px;
        }
        
        .examples code {
            display: block;
            background: white;
            padding: 10px;
            margin: 5px 0;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            color: #333;
        }
        
        .info {
            background: #e3f2fd;
            border-left: 4px solid #2196f3;
            padding: 15px;
            border-radius: 5px;
            margin-top: 20px;
        }
        
        .info strong {
            color: #1976d2;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🗄️ MiniSQL</h1>
            <p>Minimal SQL Interpreter Web Interface</p>
        </div>
        
        <div class="content">
            <div class="section">
                <h2>Execute SQL Command</h2>
                <textarea id="sqlInput" placeholder="Enter your SQL command here...
Example: CREATE TABLE users (id, name, age);">CREATE TABLE users (id, name, age);</textarea>
                <br><br>
                <button onclick="executeSql()">Execute SQL (Ctrl+Enter)</button>
            </div>
            
            <div class="section">
                <h2>Result</h2>
                <div id="result" class="result">Results will appear here...</div>
            </div>
            
            <div class="section">
                <h2>Examples</h2>
                <div class="examples">
                    <code>CREATE TABLE users (id, name, age);</code>
                    <code>INSERT INTO users VALUES (1, Alice, 30);</code>
                    <code>INSERT INTO users VALUES (2, Bob, 25);</code>
                    <code>SELECT * FROM users;</code>
                    <code>SELECT * FROM users WHERE age = 25;</code>
                </div>
            </div>
            
            <div class="info">
                <strong>Note:</strong> Tables are automatically persisted to CSV files in the data/ directory. 
                Press Ctrl+Enter to quickly execute your SQL command.
            </div>
        </div>
    </div>
    
    <script>
        function executeSql() {
            const sql = document.getElementById('sqlInput').value;
            const resultDiv = document.getElementById('result');
            
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
                resultDiv.textContent = data;
            })
            .catch(error => {
                resultDiv.textContent = 'Error: ' + error;
            });
        }
        
        // Ctrl+Enter keyboard shortcut
        document.getElementById('sqlInput').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeSql();
            }
        });
    </script>
</body>
</html>)HTML";
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else {
            result += str[i];
        }
    }
    return result;
}
