#include "HttpServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>

HttpServer::HttpServer(int port, Engine& engine) : port_(port), engine_(engine) {}

void HttpServer::start() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error opening socket\n";
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket\n";
        close(serverSocket);
        return;
    }

    listen(serverSocket, 5);
    std::cout << "Server listening on port " << port_ << "\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            std::cerr << "Error accepting connection\n";
            continue;
        }

        handleRequest(clientSocket);
        close(clientSocket);
    }

    close(serverSocket);
}

void HttpServer::handleRequest(int clientSocket) {
    char buffer[1024] = {0};
    read(clientSocket, buffer, 1024);

    // Simple parsing of GET request
    std::string request(buffer);
    size_t sqlStart = request.find("sql=");
    std::string sql;

    if (sqlStart != std::string::npos) {
        sqlStart += 4; // Skip "sql="
        size_t sqlEnd = request.find(" HTTP/", sqlStart);
        if (sqlEnd != std::string::npos) {
            sql = request.substr(sqlStart, sqlEnd - sqlStart);
        }
    }

    std::string response = buildResponse(sql);
    write(clientSocket, response.c_str(), response.size());
}

std::string HttpServer::buildResponse(const std::string& sql) {
    std::string result;
    if (!sql.empty()) {
        result = engine_.executeStatementWeb(sql);
    }

    std::string html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Minimal SQL Interpreter</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
        }
        h1 {
            color: #333;
        }
        textarea {
            width: 100%;
            height: 100px;
            margin-bottom: 10px;
        }
        button {
            padding: 8px 16px;
            background-color: #4CAF50;
            color: white;
            border: none;
            cursor: pointer;
        }
        button:hover {
            background-color: #45a049;
        }
        pre {
            background-color: #f4f4f4;
            padding: 10px;
            border-radius: 5px;
            overflow-x: auto;
        }
        .examples {
            margin-top: 20px;
            padding: 10px;
            background-color: #e9e9e9;
            border-radius: 5px;
        }
    </style>
</head>
<body>
    <h1>Minimal SQL Interpreter</h1>
    <form action="/" method="get">
        <textarea name="sql" placeholder="Enter SQL command...">)HTML" + sql + R"HTML(</textarea>
        <br>
        <button type="submit">Execute</button>
    </form>

    <div class="examples">
        <h3>Examples:</h3>
        <p>CREATE TABLE users (id, name, age);</p>
        <p>INSERT INTO users VALUES (1, Alice, 30);</p>
        <p>SELECT * FROM users;</p>
        <p>SELECT * FROM users WHERE age = 30;</p>
    </div>

    <h3>Result:</h3>
    <pre>)HTML" + result + R"HTML(</pre>
</body>
</html>
)HTML";

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(html.size()) + "\r\n";
    response += "\r\n";
    response += html;

    return response;
}
