#include "HttpServer.h"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sstream>

HttpServer::HttpServer(int port, std::function<std::string(const std::string&)> sqlExecutor)
    : port_(port), sqlExecutor_(sqlExecutor), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_) {
        return;
    }

    running_ = true;

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening on socket" << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "HTTP server started on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }

        // Create thread data
        ThreadData* threadData = new ThreadData;
        threadData->server = this;
        threadData->clientSocket = clientSocket;

        pthread_t thread;
        if (pthread_create(&thread, NULL, clientThread, threadData) != 0) {
            std::cerr << "Error creating thread" << std::endl;
            close(clientSocket);
            delete threadData;
        }
    }

    close(serverSocket);
}

void HttpServer::stop() {
    running_ = false;
}

void HttpServer::handleClient(int clientSocket) {
    char buffer[1024] = {0};
    read(clientSocket, buffer, 1024);

    std::string request(buffer);
    std::string response;

    // Check if it's a GET request
    if (request.find("GET / ") != std::string::npos) {
        // Simple HTML form for SQL commands
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "\r\n"
                   "<!DOCTYPE html>\n"
                   "<html>\n"
                   "<head>\n"
                   "    <title>Minimal SQL Interpreter</title>\n"
                   "    <style>\n"
                   "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
                   "        h1 { color: #333; }\n"
                   "        textarea { width: 100%; height: 100px; }\n"
                   "        button { padding: 8px 16px; background: #4CAF50; color: white; border: none; cursor: pointer; }\n"
                   "        button:hover { background: #45a049; }\n"
                   "        #result { margin-top: 20px; padding: 10px; border: 1px solid #ddd; }\n"
                   "    </style>\n"
                   "</head>\n"
                   "<body>\n"
                   "    <h1>Minimal SQL Interpreter</h1>\n"
                   "    <form id='sqlForm'>\n"
                   "        <label for='sqlCommand'>SQL Command:</label><br>\n"
                   "        <textarea id='sqlCommand' name='sqlCommand'></textarea><br>\n"
                   "        <button type='button' onclick='executeSQL()'>Execute</button>\n"
                   "    </form>\n"
                   "    <div id='result'></div>\n"
                   "    <script>\n"
                   "        function executeSQL() {\n"
                   "            const sqlCommand = document.getElementById('sqlCommand').value;\n"
                   "            const resultDiv = document.getElementById('result');\n"
                   "\n"
                   "            fetch('/execute', {\n"
                   "                method: 'POST',\n"
                   "                headers: {\n"
                   "                    'Content-Type': 'application/x-www-form-urlencoded',\n"
                   "                },\n"
                   "                body: 'sql=' + encodeURIComponent(sqlCommand)\n"
                   "            })\n"
                   "            .then(response => response.text())\n"
                   "            .then(data => {\n"
                   "                resultDiv.innerHTML = '<h3>Result:</h3><pre>' + data + '</pre>';\n"
                   "            })\n"
                   "            .catch(error => {\n"
                   "                resultDiv.innerHTML = '<h3>Error:</h3><pre>' + error + '</pre>';\n"
                   "            });\n"
                   "        }\n"
                   "    </script>\n"
                   "</body>\n"
                   "</html>\n";
    }
    // Check if it's a POST request to /execute
    else if (request.find("POST /execute ") != std::string::npos) {
        // Extract SQL command from POST data
        size_t sqlPos = request.find("sql=");
        if (sqlPos != std::string::npos) {
            sqlPos += 4; // Skip "sql="
            size_t endPos = request.find(" ", sqlPos);
            if (endPos == std::string::npos) {
                endPos = request.length();
            }

            std::string sqlCommand = request.substr(sqlPos, endPos - sqlPos);
            // URL decode the SQL command
            std::string decodedSql;
            for (size_t i = 0; i < sqlCommand.length(); i++) {
                if (sqlCommand[i] == '+') {
                    decodedSql += ' ';
                } else if (sqlCommand[i] == '%' && i + 2 < sqlCommand.length()) {
                    int hexValue;
                    std::string hex = sqlCommand.substr(i + 1, 2);
                    std::stringstream converter;
                    converter << std::hex << hex;
                    converter >> hexValue;
                    decodedSql += static_cast<char>(hexValue);
                    i += 2;
                } else {
                    decodedSql += sqlCommand[i];
                }
            }

            // Execute SQL command
            std::string result = sqlExecutor_(decodedSql);

            // Prepare response
            response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/plain\r\n"
                       "\r\n"
                       + result;
        } else {
            response = "HTTP/1.1 400 Bad Request\r\n"
                       "Content-Type: text/plain\r\n"
                       "\r\n"
                       "Missing SQL command";
        }
    }
    else {
        response = "HTTP/1.1 404 Not Found\r\n"
                   "Content-Type: text/plain\r\n"
                   "\r\n"
                   "404 Not Found";
    }

    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

void* HttpServer::clientThread(void* arg) {
    ThreadData* threadData = static_cast<ThreadData*>(arg);
    HttpServer* server = threadData->server;
    int clientSocket = threadData->clientSocket;

    server->handleClient(clientSocket);

    delete threadData;
    return NULL;
}
