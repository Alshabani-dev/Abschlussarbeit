#include "HttpServer.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

HttpServer::HttpServer(int port, std::function<std::string(const std::string&)> sqlExecutor)
    : port_(port), serverSocket_(-1), running_(false), sqlExecutor_(sqlExecutor) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options" << std::endl;
        close(serverSocket_);
        return;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error binding socket to port " << port_ << std::endl;
        close(serverSocket_);
        return;
    }
    
    // Listen
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket_);
        return;
    }
    
    running_ = true;
    std::cout << "HTTP Server started on http://localhost:" << port_ << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Accept connections
    while (running_) {
        struct sockaddr_in clientAddress;
        socklen_t clientLen = sizeof(clientAddress);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddress, &clientLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Error accepting connection" << std::endl;
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
    int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    
    if (bytesRead <= 0) {
        return;
    }
    
    std::string request(buffer, bytesRead);
    
    // Parse HTTP request
    std::istringstream requestStream(request);
    std::string method, path, version;
    requestStream >> method >> path >> version;
    
    if (method == "GET" && path == "/") {
        // Serve HTML page
        std::string htmlPage = getHtmlPage();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << htmlPage.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << htmlPage;
        
        std::string responseStr = response.str();
        write(clientSocket, responseStr.c_str(), responseStr.size());
    } else if (method == "POST" && path == "/execute") {
        // Extract SQL from POST body
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
            std::string result = sqlExecutor_(sql);
            result = htmlEscape(result);
            
            // Replace newlines with <br> for HTML display
            size_t pos = 0;
            while ((pos = result.find('\n', pos)) != std::string::npos) {
                result.replace(pos, 1, "<br>");
                pos += 4;
            }
            
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: text/plain\r\n";
            response << "Content-Length: " << result.size() << "\r\n";
            response << "Connection: close\r\n";
            response << "\r\n";
            response << result;
            
            std::string responseStr = response.str();
            write(clientSocket, responseStr.c_str(), responseStr.size());
        }
    } else {
        // 404 Not Found
        std::string notFound = "404 Not Found";
        std::ostringstream response;
        response << "HTTP/1.1 404 Not Found\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: " << notFound.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << notFound;
        
        std::string responseStr = response.str();
        write(clientSocket, responseStr.c_str(), responseStr.size());
    }
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
            padding: 40px;
            max-width: 900px;
            width: 100%;
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
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
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
            padding: 8px;
            margin: 5px 0;
            border-radius: 4px;
            font-family: 'Courier New', monospace;
            color: #333;
        }
        
        textarea {
            width: 100%;
            height: 150px;
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
            margin-top: 20px;
            display: flex;
            gap: 10px;
        }
        
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 8px;
            font-size: 16px;
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
        
        #result {
            margin-top: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 8px;
            min-height: 100px;
            white-space: pre-wrap;
            font-family: 'Courier New', monospace;
            border: 2px solid #e0e0e0;
            display: none;
        }
        
        #result.show {
            display: block;
        }
        
        #result.error {
            background: #fee;
            border-color: #fcc;
            color: #c33;
        }
        
        .hint {
            color: #999;
            font-size: 0.9em;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🗄️ MiniSQL</h1>
        <p class="subtitle">Web Interface for SQL Execution</p>
        
        <div class="examples">
            <h3>Example Commands:</h3>
            <code>CREATE TABLE users (id, name, age);</code>
            <code>INSERT INTO users VALUES (1, Alice, 30);</code>
            <code>SELECT * FROM users;</code>
            <code>SELECT * FROM users WHERE age = 30;</code>
        </div>
        
        <textarea id="sqlInput" placeholder="Enter your SQL command here...">CREATE TABLE users (id, name, age);</textarea>
        
        <div class="button-container">
            <button onclick="executeSQL()">Execute SQL</button>
            <button onclick="clearInput()" style="background: #6c757d;">Clear</button>
        </div>
        
        <p class="hint">💡 Press Ctrl+Enter to execute</p>
        
        <div id="result"></div>
    </div>
    
    <script>
        function executeSQL() {
            const sql = document.getElementById('sqlInput').value.trim();
            const resultDiv = document.getElementById('result');
            
            if (!sql) {
                resultDiv.className = 'show error';
                resultDiv.innerHTML = 'Please enter a SQL command';
                return;
            }
            
            resultDiv.className = 'show';
            resultDiv.innerHTML = 'Executing...';
            
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
                    resultDiv.className = 'show error';
                } else {
                    resultDiv.className = 'show';
                }
                resultDiv.innerHTML = data;
            })
            .catch(error => {
                resultDiv.className = 'show error';
                resultDiv.innerHTML = 'Request failed: ' + error;
            });
        }
        
        function clearInput() {
            document.getElementById('sqlInput').value = '';
            document.getElementById('result').className = '';
        }
        
        document.getElementById('sqlInput').addEventListener('keydown', function(e) {
            if (e.ctrlKey && e.key === 'Enter') {
                executeSQL();
            }
        });
    </script>
</body>
</html>)HTML";
}

std::string HttpServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            int value;
            std::istringstream is(str.substr(i + 1, 2));
            if (is >> std::hex >> value) {
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

std::string HttpServer::htmlEscape(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }
    return result;
}
