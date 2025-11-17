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
    : engine_(engine), port_(port), serverSocket_(-1) {}

void HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }
    
    // Allow socket reuse
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind socket
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << "\n";
        close(serverSocket_);
        return;
    }
    
    // Listen
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Failed to listen on socket\n";
        close(serverSocket_);
        return;
    }
    
    std::cout << "HTTP Server started on http://localhost:" << port_ << "\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    // Accept connections
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            continue;
        }
        
        handleClient(clientSocket);
        close(clientSocket);
    }
    
    close(serverSocket_);
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        return;
    }
    
    std::string request(buffer);
    std::string method, path;
    std::string body = parseHttpRequest(request, method, path);
    
    std::string response;
    
    if (method == "GET" && path == "/") {
        // Serve index page
        response = createHttpResponse(getIndexHtml(), "text/html");
    }
    else if (method == "POST" && path == "/execute") {
        // Execute SQL
        std::string sql = urlDecode(body);
        
        // Remove "sql=" prefix if present
        if (sql.substr(0, 4) == "sql=") {
            sql = sql.substr(4);
        }
        
        std::string result = engine_->executeStatementWeb(sql);
        response = createHttpResponse(result, "text/plain");
    }
    else {
        // 404
        std::string content = "404 Not Found";
        response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
        response += "\r\n";
        response += content;
    }
    
    send(clientSocket, response.c_str(), response.length(), 0);
}

std::string HttpServer::parseHttpRequest(const std::string& request, std::string& method, std::string& path) {
    std::istringstream iss(request);
    std::string line;
    
    // Parse first line: METHOD PATH HTTP/1.1
    if (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> path;
    }
    
    // Find body (after empty line)
    std::string body;
    bool inBody = false;
    while (std::getline(iss, line)) {
        if (inBody) {
            body += line;
        } else if (line == "\r" || line.empty()) {
            inBody = true;
        }
    }
    
    return body;
}

std::string HttpServer::createHttpResponse(const std::string& content, const std::string& contentType) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "\r\n";
    response += content;
    return response;
}

std::string HttpServer::getIndexHtml() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MinSQL Interpreter</title>
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
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
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
        
        #sqlInput {
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
        
        #sqlInput:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .button-group {
            display: flex;
            gap: 10px;
            margin-top: 15px;
        }
        
        button {
            padding: 12px 30px;
            border: none;
            border-radius: 6px;
            font-size: 16px;
            cursor: pointer;
            transition: all 0.3s;
            font-weight: 600;
        }
        
        #executeBtn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            flex: 1;
        }
        
        #executeBtn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        #clearBtn {
            background: #f0f0f0;
            color: #333;
        }
        
        #clearBtn:hover {
            background: #e0e0e0;
        }
        
        #result {
            background: #f8f9fa;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            padding: 20px;
            min-height: 150px;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            white-space: pre-wrap;
            word-wrap: break-word;
            max-height: 400px;
            overflow-y: auto;
        }
        
        .examples {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }
        
        .examples h3 {
            color: #667eea;
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        
        .examples code {
            display: block;
            background: white;
            padding: 10px;
            margin: 8px 0;
            border-radius: 4px;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            border: 1px solid #e0e0e0;
        }
        
        .shortcut {
            background: #fff3cd;
            padding: 12px;
            border-radius: 6px;
            margin-top: 15px;
            border-left: 4px solid #ffc107;
            font-size: 14px;
        }
        
        .shortcut strong {
            color: #856404;
        }
        
        .loading {
            color: #667eea;
            font-style: italic;
        }
        
        .error {
            color: #dc3545;
        }
        
        .success {
            color: #28a745;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>⚡ MinSQL Interpreter</h1>
            <p>Execute SQL commands in your browser</p>
        </div>
        
        <div class="content">
            <div class="section">
                <h2>SQL Command</h2>
                <textarea id="sqlInput" placeholder="Enter your SQL command here...&#10;Example: CREATE TABLE users (id, name, age);"></textarea>
                <div class="button-group">
                    <button id="executeBtn">Execute SQL</button>
                    <button id="clearBtn">Clear</button>
                </div>
                <div class="shortcut">
                    <strong>💡 Tip:</strong> Press <kbd>Ctrl+Enter</kbd> to execute your SQL command quickly!
                </div>
            </div>
            
            <div class="section">
                <h2>Result</h2>
                <div id="result">Ready to execute SQL commands...</div>
            </div>
            
            <div class="section">
                <div class="examples">
                    <h3>📝 Example Commands</h3>
                    <code>CREATE TABLE users (id, name, age);</code>
                    <code>INSERT INTO users VALUES (1, Alice, 30);</code>
                    <code>INSERT INTO users VALUES (2, Bob, 25);</code>
                    <code>SELECT * FROM users;</code>
                    <code>SELECT * FROM users WHERE age = 25;</code>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        const sqlInput = document.getElementById('sqlInput');
        const executeBtn = document.getElementById('executeBtn');
        const clearBtn = document.getElementById('clearBtn');
        const result = document.getElementById('result');
        
        async function executeSql() {
            const sql = sqlInput.value.trim();
            if (!sql) {
                result.textContent = 'Please enter a SQL command.';
                result.className = 'error';
                return;
            }
            
            result.textContent = 'Executing...';
            result.className = 'loading';
            
            try {
                const response = await fetch('/execute', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    },
                    body: 'sql=' + encodeURIComponent(sql)
                });
                
                const text = await response.text();
                result.textContent = text;
                
                if (text.startsWith('Error:')) {
                    result.className = 'error';
                } else if (text === 'OK') {
                    result.className = 'success';
                } else {
                    result.className = '';
                }
            } catch (error) {
                result.textContent = 'Error: ' + error.message;
                result.className = 'error';
            }
        }
        
        executeBtn.addEventListener('click', executeSql);
        
        clearBtn.addEventListener('click', () => {
            sqlInput.value = '';
            result.textContent = 'Ready to execute SQL commands...';
            result.className = '';
            sqlInput.focus();
        });
        
        // Ctrl+Enter to execute
        sqlInput.addEventListener('keydown', (e) => {
            if (e.ctrlKey && e.key === 'Enter') {
                e.preventDefault();
                executeSql();
            }
        });
        
        // Focus on load
        sqlInput.focus();
    </script>
</body>
</html>)HTML";
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
