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
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: Failed to set socket options" << std::endl;
        close(serverSocket_);
        return;
    }
    
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
    std::cout << "HTTP server running on http://localhost:" << port_ << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddress;
        socklen_t clientLen = sizeof(clientAddress);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientLen);
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
    
    // Parse request
    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;
    
    std::string response;
    
    if (method == "GET" && path == "/") {
        // Serve HTML page
        response = createResponse(getHtmlPage());
    }
    else if (method == "POST" && path == "/execute") {
        // Extract SQL from POST body
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            std::string body = request.substr(bodyStart + 4);
            
            // Parse form data (sql=...)
            std::string sql;
            if (body.substr(0, 4) == "sql=") {
                sql = urlDecode(body.substr(4));
            }
            
            // Execute SQL
            std::string result = engine_->executeStatementWeb(sql);
            
            // Return result as plain text
            response = createResponse(result, "text/plain");
        } else {
            response = createResponse("Error: No body in POST request", "text/plain");
        }
    }
    else {
        // 404 Not Found
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
    }
    
    write(clientSocket, response.c_str(), response.length());
}

std::string HttpServer::parseRequest(const std::string& request) {
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart == std::string::npos) {
        return "";
    }
    return request.substr(bodyStart + 4);
}

std::string HttpServer::createResponse(const std::string& content, const std::string& contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "\r\n";
    response << content;
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
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            max-width: 900px;
            width: 100%;
            padding: 40px;
        }
        
        h1 {
            color: #667eea;
            margin-bottom: 10px;
            font-size: 2.5em;
        }
        
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 1.1em;
        }
        
        .examples {
            background: #f8f9fa;
            border-left: 4px solid #667eea;
            padding: 15px;
            margin-bottom: 25px;
            border-radius: 5px;
        }
        
        .examples h3 {
            color: #667eea;
            margin-bottom: 10px;
            font-size: 1.2em;
        }
        
        .examples code {
            display: block;
            background: white;
            padding: 8px 12px;
            margin: 5px 0;
            border-radius: 4px;
            font-family: 'Courier New', monospace;
            color: #333;
            font-size: 0.9em;
        }
        
        .input-group {
            margin-bottom: 20px;
        }
        
        label {
            display: block;
            margin-bottom: 10px;
            color: #333;
            font-weight: 600;
            font-size: 1.1em;
        }
        
        textarea {
            width: 100%;
            padding: 15px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 16px;
            font-family: 'Courier New', monospace;
            resize: vertical;
            min-height: 120px;
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
            padding: 15px 40px;
            font-size: 1.1em;
            border-radius: 8px;
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
        
        .output {
            background: #f8f9fa;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            padding: 20px;
            margin-top: 20px;
            white-space: pre-wrap;
            font-family: 'Courier New', monospace;
            min-height: 100px;
            max-height: 400px;
            overflow-y: auto;
            line-height: 1.6;
        }
        
        .output.error {
            background: #fff5f5;
            border-color: #fc8181;
            color: #c53030;
        }
        
        .output.success {
            background: #f0fff4;
            border-color: #68d391;
            color: #22543d;
        }
        
        .shortcut-hint {
            color: #999;
            font-size: 0.9em;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🗄️ MiniSQL</h1>
        <p class="subtitle">Web-based SQL Interpreter</p>
        
        <div class="examples">
            <h3>Example Commands:</h3>
            <code>CREATE TABLE users (id, name, age);</code>
            <code>INSERT INTO users VALUES (1, Alice, 30);</code>
            <code>SELECT * FROM users;</code>
            <code>SELECT * FROM users WHERE age = 30;</code>
        </div>
        
        <div class="input-group">
            <label for="sqlInput">Enter SQL Command:</label>
            <textarea id="sqlInput" placeholder="Type your SQL command here..."></textarea>
            <p class="shortcut-hint">Press Ctrl+Enter to execute</p>
        </div>
        
        <button onclick="executeSQL()">Execute SQL</button>
        
        <div id="output" class="output">Results will appear here...</div>
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
        
        function executeSQL() {
            const sql = sqlInput.value.trim();
            
            if (!sql) {
                output.textContent = 'Error: Please enter a SQL command';
                output.className = 'output error';
                return;
            }
            
            output.textContent = 'Executing...';
            output.className = 'output';
            
            fetch('/execute', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'sql=' + encodeURIComponent(sql)
            })
            .then(response => response.text())
            .then(result => {
                output.textContent = result;
                
                if (result.startsWith('Error:')) {
                    output.className = 'output error';
                } else if (result === 'OK') {
                    output.className = 'output success';
                } else {
                    output.className = 'output';
                }
            })
            .catch(error => {
                output.textContent = 'Error: ' + error.message;
                output.className = 'output error';
            });
        }
    </script>
</body>
</html>)HTML";
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    char ch;
    int i, ii;
    
    for (i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            result += ch;
            i = i + 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    
    return result;
}
