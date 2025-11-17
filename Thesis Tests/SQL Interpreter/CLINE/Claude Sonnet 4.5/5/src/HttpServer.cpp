#include "HttpServer.h"
#include "Utils.h"
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

bool HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error: Failed to create socket" << std::endl;
        return false;
    }
    
    // Allow socket reuse
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Warning: Failed to set SO_REUSEADDR" << std::endl;
    }
    
    // Bind socket
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Failed to bind to port " << port_ << std::endl;
        close(serverSocket_);
        return false;
    }
    
    // Listen
    if (listen(serverSocket_, 5) < 0) {
        std::cerr << "Error: Failed to listen" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    running_ = true;
    std::cout << "HTTP server started on http://localhost:" << port_ << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error: Failed to accept connection" << std::endl;
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
    
    // Parse request
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;
    
    std::string response;
    
    if (method == "GET" && path == "/") {
        // Serve main page
        std::string html = getHtmlPage();
        response = createHttpResponse(html);
    } else if (method == "POST" && path == "/execute") {
        // Execute SQL command
        std::string sql = parseHttpRequest(request);
        std::string result = engine_->executeStatementWeb(sql);
        
        // Escape HTML special characters
        std::string escapedResult;
        for (char ch : result) {
            if (ch == '<') {
                escapedResult += "&lt;";
            } else if (ch == '>') {
                escapedResult += "&gt;";
            } else if (ch == '&') {
                escapedResult += "&amp;";
            } else if (ch == '"') {
                escapedResult += "&quot;";
            } else {
                escapedResult += ch;
            }
        }
        
        response = createHttpResponse(escapedResult, "text/plain");
    } else {
        // 404 Not Found
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }
    
    send(clientSocket, response.c_str(), response.length(), 0);
}

std::string HttpServer::parseHttpRequest(const std::string& request) {
    // Find the body (after double newline)
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        return "";
    }
    
    std::string body = request.substr(bodyStart + 4);
    
    // Parse form data (sql=...)
    std::string prefix = "sql=";
    size_t sqlStart = body.find(prefix);
    if (sqlStart == std::string::npos) {
        return "";
    }
    
    std::string sql = body.substr(sqlStart + prefix.length());
    
    // URL decode
    std::string decoded;
    for (size_t i = 0; i < sql.length(); ++i) {
        if (sql[i] == '%' && i + 2 < sql.length()) {
            // Hex decode
            char hex[3] = {sql[i + 1], sql[i + 2], '\0'};
            char ch = static_cast<char>(strtol(hex, nullptr, 16));
            decoded += ch;
            i += 2;
        } else if (sql[i] == '+') {
            decoded += ' ';
        } else {
            decoded += sql[i];
        }
    }
    
    return decoded;
}

std::string HttpServer::getHtmlPage() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MiniSQL - SQL Interpreter</title>
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
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
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
        
        .input-section {
            margin-bottom: 20px;
        }
        
        .input-section label {
            display: block;
            font-weight: 600;
            color: #333;
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        
        .input-section textarea {
            width: 100%;
            padding: 15px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 14px;
            resize: vertical;
            min-height: 120px;
            transition: border-color 0.3s;
        }
        
        .input-section textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .examples {
            background: #f8f9fa;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
        }
        
        .examples h3 {
            color: #333;
            margin-bottom: 10px;
            font-size: 1em;
        }
        
        .examples code {
            display: block;
            background: white;
            padding: 8px;
            border-radius: 4px;
            margin: 5px 0;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            border-left: 3px solid #667eea;
        }
        
        .button-group {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
        }
        
        button {
            flex: 1;
            padding: 15px;
            font-size: 16px;
            font-weight: 600;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s;
        }
        
        .execute-btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        
        .execute-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        .clear-btn {
            background: #e0e0e0;
            color: #333;
        }
        
        .clear-btn:hover {
            background: #d0d0d0;
        }
        
        .result-section {
            margin-top: 20px;
        }
        
        .result-section h3 {
            color: #333;
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        
        .result-box {
            background: #f8f9fa;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            padding: 15px;
            min-height: 100px;
            max-height: 400px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            white-space: pre-wrap;
            word-wrap: break-word;
        }
        
        .result-box.success {
            border-color: #4caf50;
            background: #f1f8f4;
        }
        
        .result-box.error {
            border-color: #f44336;
            background: #ffebee;
            color: #c62828;
        }
        
        .shortcut-hint {
            text-align: center;
            color: #666;
            font-size: 0.9em;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🗄️ MiniSQL</h1>
            <p>Pure C++ SQL Interpreter with Persistence</p>
        </div>
        
        <div class="content">
            <div class="examples">
                <h3>📝 Example Commands:</h3>
                <code>CREATE TABLE users (id, name, age);</code>
                <code>INSERT INTO users VALUES (1, Alice, 30);</code>
                <code>SELECT * FROM users WHERE age = 30;</code>
            </div>
            
            <div class="input-section">
                <label for="sqlInput">SQL Command:</label>
                <textarea id="sqlInput" placeholder="Enter your SQL command here..."></textarea>
            </div>
            
            <div class="button-group">
                <button class="execute-btn" onclick="executeSQL()">Execute SQL</button>
                <button class="clear-btn" onclick="clearAll()">Clear</button>
            </div>
            
            <div class="shortcut-hint">💡 Press Ctrl+Enter to execute</div>
            
            <div class="result-section">
                <h3>Result:</h3>
                <div id="result" class="result-box">Results will appear here...</div>
            </div>
        </div>
    </div>
    
    <script>
        const sqlInput = document.getElementById('sqlInput');
        const resultBox = document.getElementById('result');
        
        // Keyboard shortcut: Ctrl+Enter to execute
        sqlInput.addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeSQL();
            }
        });
        
        function executeSQL() {
            const sql = sqlInput.value.trim();
            
            if (!sql) {
                resultBox.textContent = 'Please enter a SQL command.';
                resultBox.className = 'result-box error';
                return;
            }
            
            resultBox.textContent = 'Executing...';
            resultBox.className = 'result-box';
            
            fetch('/execute', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'sql=' + encodeURIComponent(sql)
            })
            .then(response => response.text())
            .then(data => {
                resultBox.textContent = data;
                if (data.startsWith('Error') || data.startsWith('Parse error')) {
                    resultBox.className = 'result-box error';
                } else {
                    resultBox.className = 'result-box success';
                }
            })
            .catch(error => {
                resultBox.textContent = 'Network error: ' + error.message;
                resultBox.className = 'result-box error';
            });
        }
        
        function clearAll() {
            sqlInput.value = '';
            resultBox.textContent = 'Results will appear here...';
            resultBox.className = 'result-box';
        }
    </script>
</body>
</html>)HTML";
}

std::string HttpServer::createHttpResponse(const std::string& body, const std::string& contentType) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    oss << "Content-Length: " << body.length() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;
    return oss.str();
}
