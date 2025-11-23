#include "HttpServer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

HttpServer::HttpServer(int port) 
    : port_(port), serverSocket_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "ERROR: Failed to create socket\n";
        return false;
    }
    
    // Allow reuse of address
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "ERROR: Failed to bind to port " << port_ << "\n";
        close(serverSocket_);
        return false;
    }
    
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "ERROR: Failed to listen on socket\n";
        close(serverSocket_);
        return false;
    }
    
    running_ = true;
    std::cout << "HTTP Server started on http://localhost:" << port_ << "\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "ERROR: Failed to accept connection\n";
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
    
    std::string method, path;
    std::string body = parseRequest(request, method, path);
    
    std::string response;
    
    if (method == "GET" && path == "/") {
        // Serve index page
        response = buildResponse(200, "text/html", getIndexHtml());
    }
    else if (method == "POST" && path == "/execute") {
        // Execute SQL
        std::string sql = urlDecode(body);
        if (sql.substr(0, 4) == "sql=") {
            sql = sql.substr(4);
        }
        
        std::string result = engine_.executeStatementWeb(sql);
        
        // Escape HTML characters in result
        std::string escapedResult;
        for (char c : result) {
            switch (c) {
                case '<': escapedResult += "&lt;"; break;
                case '>': escapedResult += "&gt;"; break;
                case '&': escapedResult += "&amp;"; break;
                case '"': escapedResult += "&quot;"; break;
                default: escapedResult += c;
            }
        }
        
        response = buildResponse(200, "text/plain", escapedResult);
    }
    else {
        response = buildResponse(404, "text/plain", "Not Found");
    }
    
    send(clientSocket, response.c_str(), response.length(), 0);
}

std::string HttpServer::parseRequest(const std::string& request, std::string& method, std::string& path) {
    std::istringstream iss(request);
    std::string line;
    
    // Parse request line
    if (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> path;
    }
    
    // Find body (after blank line)
    std::string body;
    bool foundBlankLine = false;
    while (std::getline(iss, line)) {
        if (line == "\r" || line.empty()) {
            foundBlankLine = true;
            break;
        }
    }
    
    if (foundBlankLine) {
        std::string remaining((std::istreambuf_iterator<char>(iss)), 
                             std::istreambuf_iterator<char>());
        body = remaining;
    }
    
    return body;
}

std::string HttpServer::buildResponse(int statusCode, const std::string& contentType, const std::string& body) {
    std::ostringstream oss;
    
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "Unknown";
    }
    
    oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.length() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;
    
    return oss.str();
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
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
        }
        
        .container {
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
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
        
        .examples {
            background: #f8f9fa;
            border-left: 4px solid #667eea;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 5px;
        }
        
        .examples h3 {
            color: #667eea;
            margin-bottom: 10px;
        }
        
        .examples code {
            display: block;
            background: white;
            padding: 8px;
            margin: 5px 0;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
            color: #333;
        }
        
        .input-group {
            margin-bottom: 20px;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #333;
        }
        
        textarea {
            width: 100%;
            min-height: 120px;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 5px;
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
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
        }
        
        button {
            flex: 1;
            padding: 12px 24px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 5px;
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
        
        button:disabled {
            background: #ccc;
            cursor: not-allowed;
            transform: none;
        }
        
        .output {
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 20px;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            font-size: 14px;
            white-space: pre-wrap;
            min-height: 200px;
            max-height: 400px;
            overflow-y: auto;
        }
        
        .output.error {
            color: #f48771;
        }
        
        .output.success {
            color: #a9dc76;
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
            <p>Web-Based SQL Interpreter</p>
        </div>
        
        <div class="content">
            <div class="examples">
                <h3>Example Commands:</h3>
                <code>CREATE TABLE users (id, name, age);</code>
                <code>INSERT INTO users VALUES (1, Alice, 30);</code>
                <code>SELECT * FROM users;</code>
                <code>SELECT * FROM users WHERE age = 30;</code>
            </div>
            
            <div class="input-group">
                <label for="sqlInput">SQL Command:</label>
                <textarea id="sqlInput" placeholder="Enter your SQL command here..."></textarea>
            </div>
            
            <div class="button-group">
                <button onclick="executeSQL()">▶ Execute SQL</button>
                <button onclick="clearAll()">🗑️ Clear All</button>
            </div>
            
            <div class="shortcut-hint">
                💡 Press Ctrl+Enter to execute
            </div>
            
            <div class="input-group">
                <label>Output:</label>
                <div id="output" class="output">Ready to execute SQL commands...</div>
            </div>
        </div>
    </div>
    
    <script>
        const sqlInput = document.getElementById('sqlInput');
        const output = document.getElementById('output');
        
        // Ctrl+Enter to execute
        sqlInput.addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeSQL();
            }
        });
        
        async function executeSQL() {
            const sql = sqlInput.value.trim();
            
            if (!sql) {
                output.textContent = 'Please enter a SQL command.';
                output.className = 'output error';
                return;
            }
            
            output.textContent = 'Executing...';
            output.className = 'output';
            
            try {
                const response = await fetch('/execute', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded',
                    },
                    body: 'sql=' + encodeURIComponent(sql)
                });
                
                const result = await response.text();
                output.textContent = result;
                
                if (result.startsWith('ERROR')) {
                    output.className = 'output error';
                } else {
                    output.className = 'output success';
                }
            } catch (error) {
                output.textContent = 'ERROR: Failed to connect to server\n' + error.message;
                output.className = 'output error';
            }
        }
        
        function clearAll() {
            sqlInput.value = '';
            output.textContent = 'Ready to execute SQL commands...';
            output.className = 'output';
        }
    </script>
</body>
</html>)HTML";
}
