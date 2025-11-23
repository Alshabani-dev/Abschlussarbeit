#include "HttpServer.h"
#include "Utils.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>

HttpServer::HttpServer(int port) : port_(port), serverSocket_(-1), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error creating socket\n";
        return;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options\n";
        close(serverSocket_);
        return;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error binding socket to port " << port_ << "\n";
        close(serverSocket_);
        return;
    }
    
    // Listen
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Error listening on socket\n";
        close(serverSocket_);
        return;
    }
    
    running_ = true;
    std::cout << "Server started on http://localhost:" << port_ << "\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error accepting connection\n";
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
    char buffer[4096] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    
    if (bytesRead <= 0) {
        return;
    }
    
    std::string request(buffer, bytesRead);
    
    // Parse request
    std::istringstream requestStream(request);
    std::string method, path, version;
    requestStream >> method >> path >> version;
    
    std::string response;
    
    if (method == "GET" && path == "/") {
        // Serve HTML page
        response = createHttpResponse(getHtmlPage());
    } else if (method == "POST" && path == "/execute") {
        // Execute SQL command
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            std::string body = request.substr(bodyStart + 4);
            
            // Parse form data (sql=...)
            std::string sql;
            size_t sqlPos = body.find("sql=");
            if (sqlPos != std::string::npos) {
                sql = body.substr(sqlPos + 4);
                size_t endPos = sql.find('&');
                if (endPos != std::string::npos) {
                    sql = sql.substr(0, endPos);
                }
                sql = urlDecode(sql);
            }
            
            // Execute SQL
            std::string result = engine_.executeStatementWeb(sql);
            
            // Return result as plain text
            response = createHttpResponse(result, "text/plain");
        } else {
            response = createHttpResponse("Error: No SQL command provided", "text/plain");
        }
    } else {
        // 404 Not Found
        response = createHttpResponse("<html><body><h1>404 Not Found</h1></body></html>");
    }
    
    write(clientSocket, response.c_str(), response.length());
}

std::string HttpServer::createHttpResponse(const std::string& body, const std::string& contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
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
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            max-width: 900px;
            width: 100%;
            padding: 40px;
        }
        
        h1 {
            color: #667eea;
            margin-bottom: 10px;
            font-size: 2.5em;
            text-align: center;
        }
        
        .subtitle {
            color: #666;
            text-align: center;
            margin-bottom: 30px;
            font-size: 1.1em;
        }
        
        .examples {
            background: #f8f9fa;
            border-left: 4px solid #667eea;
            padding: 15px;
            margin-bottom: 25px;
            border-radius: 4px;
        }
        
        .examples h3 {
            color: #333;
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        
        .examples code {
            display: block;
            background: white;
            padding: 8px 12px;
            margin: 5px 0;
            border-radius: 4px;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
            color: #d63384;
        }
        
        .input-section {
            margin-bottom: 20px;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
        }
        
        textarea {
            width: 100%;
            min-height: 120px;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            font-family: 'Courier New', monospace;
            font-size: 1em;
            resize: vertical;
            transition: border-color 0.3s;
        }
        
        textarea:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .button-container {
            display: flex;
            gap: 10px;
            margin-bottom: 25px;
        }
        
        button {
            flex: 1;
            padding: 12px 24px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
        }
        
        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
        }
        
        button:active {
            transform: translateY(0);
        }
        
        .clear-btn {
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        }
        
        .output-section {
            margin-top: 25px;
        }
        
        .output-box {
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 20px;
            border-radius: 6px;
            font-family: 'Courier New', monospace;
            font-size: 0.9em;
            white-space: pre-wrap;
            word-wrap: break-word;
            max-height: 400px;
            overflow-y: auto;
            border: 2px solid #667eea;
        }
        
        .output-box:empty::before {
            content: 'Query results will appear here...';
            color: #888;
            font-style: italic;
        }
        
        .hint {
            margin-top: 15px;
            padding: 10px;
            background: #fff3cd;
            border-left: 4px solid #ffc107;
            border-radius: 4px;
            font-size: 0.9em;
            color: #856404;
        }
        
        @media (max-width: 600px) {
            .container {
                padding: 20px;
            }
            
            h1 {
                font-size: 2em;
            }
            
            .button-container {
                flex-direction: column;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🗄️ MiniSQL</h1>
        <p class="subtitle">Web-Based SQL Interpreter</p>
        
        <div class="examples">
            <h3>📝 Example Commands:</h3>
            <code>CREATE TABLE users (id, name, age);</code>
            <code>INSERT INTO users VALUES (1, Alice, 30);</code>
            <code>SELECT * FROM users;</code>
            <code>SELECT * FROM users WHERE age = 30;</code>
        </div>
        
        <div class="input-section">
            <label for="sqlInput">Enter SQL Command:</label>
            <textarea id="sqlInput" placeholder="Type your SQL command here..."></textarea>
        </div>
        
        <div class="button-container">
            <button onclick="executeSQL()">▶️ Execute (Ctrl+Enter)</button>
            <button class="clear-btn" onclick="clearOutput()">🗑️ Clear Output</button>
        </div>
        
        <div class="output-section">
            <label>Output:</label>
            <div id="output" class="output-box"></div>
        </div>
        
        <div class="hint">
            💡 <strong>Tip:</strong> Press <kbd>Ctrl+Enter</kbd> to quickly execute your SQL command!
        </div>
    </div>
    
    <script>
        function executeSQL() {
            const sql = document.getElementById('sqlInput').value.trim();
            const outputDiv = document.getElementById('output');
            
            if (!sql) {
                outputDiv.textContent = 'Error: Please enter a SQL command';
                return;
            }
            
            outputDiv.textContent = 'Executing...';
            
            fetch('/execute', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'sql=' + encodeURIComponent(sql)
            })
            .then(response => response.text())
            .then(data => {
                outputDiv.textContent = data;
            })
            .catch(error => {
                outputDiv.textContent = 'Error: ' + error.message;
            });
        }
        
        function clearOutput() {
            document.getElementById('output').textContent = '';
        }
        
        // Keyboard shortcut: Ctrl+Enter to execute
        document.getElementById('sqlInput').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                e.preventDefault();
                executeSQL();
            }
        });
        
        // Focus on textarea on page load
        window.onload = function() {
            document.getElementById('sqlInput').focus();
        };
    </script>
</body>
</html>
)HTML";
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.length()) {
            std::string hex = str.substr(i + 1, 2);
            char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += ch;
            i += 2;
        } else {
            result += str[i];
        }
    }
    return result;
}
