#include "HttpServer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

HttpServer::HttpServer(int port)
    : port_(port), serverSocket_(-1), running_(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

HttpServer::~HttpServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServer::setRequestHandler(std::function<std::string(const std::string&)> handler) {
    requestHandler_ = handler;
}

void HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    // Bind to port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << "\n";
#ifndef _WIN32
        close(serverSocket_);
#else
        closesocket(serverSocket_);
#endif
        return;
    }
    
    // Listen
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Failed to listen on socket\n";
#ifndef _WIN32
        close(serverSocket_);
#else
        closesocket(serverSocket_);
#endif
        return;
    }
    
    running_ = true;
    std::cout << "HTTP server running on http://localhost:" << port_ << "\n";
    std::cout << "Press Ctrl+C to stop\n";
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection\n";
            }
            continue;
        }
        
        // Handle in new thread
        std::thread(&HttpServer::handleClient, this, clientSocket).detach();
    }
}

void HttpServer::stop() {
    running_ = false;
    if (serverSocket_ >= 0) {
#ifndef _WIN32
        close(serverSocket_);
#else
        closesocket(serverSocket_);
#endif
        serverSocket_ = -1;
    }
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[4096] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesRead <= 0) {
#ifndef _WIN32
        close(clientSocket);
#else
        closesocket(clientSocket);
#endif
        return;
    }
    
    std::string request(buffer, bytesRead);
    std::string response;
    
    // Parse request method and path
    std::istringstream requestStream(request);
    std::string method, path;
    requestStream >> method >> path;
    
    if (method == "GET" && path == "/") {
        // Serve index page
        response = buildHttpResponse(getIndexHtml(), "text/html");
    } else if (method == "POST" && path == "/execute") {
        // Execute SQL
        std::string sql = parsePostData(request);
        std::string result = requestHandler_ ? requestHandler_(sql) : "No handler set";
        response = buildHttpResponse(result, "text/plain");
    } else {
        // 404
        response = buildHttpResponse("404 Not Found", "text/plain");
    }
    
    send(clientSocket, response.c_str(), response.length(), 0);
    
#ifndef _WIN32
    close(clientSocket);
#else
    closesocket(clientSocket);
#endif
}

std::string HttpServer::parsePostData(const std::string& request) {
    // Find the body (after \r\n\r\n)
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        return "";
    }
    bodyStart += 4;
    
    std::string body = request.substr(bodyStart);
    
    // Parse form data (sql=...)
    size_t sqlPos = body.find("sql=");
    if (sqlPos == std::string::npos) {
        return body; // Return whole body if not form-encoded
    }
    
    std::string sql = body.substr(sqlPos + 4);
    
    // URL decode
    std::string decoded;
    for (size_t i = 0; i < sql.length(); ++i) {
        if (sql[i] == '%' && i + 2 < sql.length()) {
            int hex;
            std::istringstream(sql.substr(i + 1, 2)) >> std::hex >> hex;
            decoded += static_cast<char>(hex);
            i += 2;
        } else if (sql[i] == '+') {
            decoded += ' ';
        } else {
            decoded += sql[i];
        }
    }
    
    return decoded;
}

std::string HttpServer::buildHttpResponse(const std::string& content, const std::string& contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "\r\n";
    response << content;
    return response.str();
}

std::string HttpServer::getIndexHtml() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MiniSQL Interpreter</title>
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
            color: #333;
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
            opacity: 0.9;
            font-size: 1.1em;
        }
        
        .content {
            padding: 30px;
        }
        
        .examples {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 25px;
            border-left: 4px solid #667eea;
        }
        
        .examples h3 {
            color: #667eea;
            margin-bottom: 15px;
        }
        
        .example-code {
            background: #2d3748;
            color: #e2e8f0;
            padding: 12px;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            margin: 8px 0;
            overflow-x: auto;
        }
        
        .input-section {
            margin-bottom: 20px;
        }
        
        .input-section label {
            display: block;
            margin-bottom: 10px;
            font-weight: 600;
            color: #667eea;
            font-size: 1.1em;
        }
        
        #sqlInput {
            width: 100%;
            padding: 15px;
            border: 2px solid #e2e8f0;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 14px;
            resize: vertical;
            min-height: 100px;
            transition: border-color 0.3s;
        }
        
        #sqlInput:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .button-group {
            display: flex;
            gap: 10px;
            margin-bottom: 20px;
        }
        
        button {
            padding: 12px 30px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            font-weight: 600;
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
        
        .output-section {
            background: #f8f9fa;
            border-radius: 8px;
            padding: 20px;
            border: 2px solid #e2e8f0;
        }
        
        .output-section h3 {
            color: #667eea;
            margin-bottom: 15px;
        }
        
        #output {
            background: #2d3748;
            color: #e2e8f0;
            padding: 15px;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            white-space: pre-wrap;
            min-height: 150px;
            overflow-x: auto;
        }
        
        .shortcut-hint {
            margin-top: 10px;
            font-size: 0.9em;
            color: #718096;
            font-style: italic;
        }
        
        .error {
            color: #f5576c;
        }
        
        .success {
            color: #48bb78;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🗄️ MiniSQL Interpreter</h1>
            <p>Execute SQL commands in your browser</p>
        </div>
        
        <div class="content">
            <div class="examples">
                <h3>📚 Example Commands</h3>
                <div class="example-code">CREATE TABLE users (id, name, age);</div>
                <div class="example-code">INSERT INTO users VALUES (1, "Alice", 30);</div>
                <div class="example-code">SELECT * FROM users;</div>
                <div class="example-code">SELECT * FROM users WHERE age = 30;</div>
            </div>
            
            <div class="input-section">
                <label for="sqlInput">✏️ SQL Command:</label>
                <textarea id="sqlInput" placeholder="Enter your SQL command here..."></textarea>
                <div class="shortcut-hint">💡 Tip: Press Ctrl+Enter to execute</div>
            </div>
            
            <div class="button-group">
                <button onclick="executeSQL()">▶️ Execute</button>
                <button class="clear-btn" onclick="clearAll()">🗑️ Clear</button>
            </div>
            
            <div class="output-section">
                <h3>📊 Output:</h3>
                <div id="output">Ready to execute SQL commands...</div>
            </div>
        </div>
    </div>
    
    <script>
        const sqlInput = document.getElementById('sqlInput');
        const output = document.getElementById('output');
        
        // Keyboard shortcut: Ctrl+Enter to execute
        sqlInput.addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                e.preventDefault();
                executeSQL();
            }
        });
        
        async function executeSQL() {
            const sql = sqlInput.value.trim();
            
            if (!sql) {
                output.textContent = 'Please enter a SQL command.';
                output.className = 'error';
                return;
            }
            
            output.textContent = 'Executing...';
            output.className = '';
            
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
                
                if (result.startsWith('Error') || result.includes('error')) {
                    output.className = 'error';
                } else {
                    output.className = 'success';
                }
            } catch (error) {
                output.textContent = 'Network error: ' + error.message;
                output.className = 'error';
            }
        }
        
        function clearAll() {
            sqlInput.value = '';
            output.textContent = 'Ready to execute SQL commands...';
            output.className = '';
            sqlInput.focus();
        }
        
        // Focus input on load
        window.onload = function() {
            sqlInput.focus();
        };
    </script>
</body>
</html>)HTML";
}
