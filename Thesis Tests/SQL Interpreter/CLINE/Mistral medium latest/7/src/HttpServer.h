#pragma once

#include <string>
#include <functional>

class HttpServer {
public:
    HttpServer(int port, std::function<std::string(const std::string&)> sqlExecutor);
    ~HttpServer();

    void start();
    void stop();

private:
    int port_;
    std::function<std::string(const std::string&)> sqlExecutor_;
    bool running_;

    void handleClient(int clientSocket);
    static void* clientThread(void* arg);

    struct ThreadData {
        HttpServer* server;
        int clientSocket;
    };
};
