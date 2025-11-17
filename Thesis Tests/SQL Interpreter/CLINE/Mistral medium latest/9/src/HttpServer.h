#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <functional>

class HttpServer {
public:
    HttpServer(int port, std::function<std::string(const std::string&)> sqlExecutor);
    void start();
    void stop();

private:
    int port_;
    std::function<std::string(const std::string&)> sqlExecutor_;
    bool running_;

    void handleClient(int clientSocket);
    void sendResponse(int clientSocket, int statusCode, const std::string& statusText);
    void sendResponse(int clientSocket, int statusCode, const std::string& statusText, const std::string& content);
};

#endif // HTTPSERVER_H
