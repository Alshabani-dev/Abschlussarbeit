#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>

class HttpServer {
public:
    HttpServer(int port);
    ~HttpServer();

    void start();
    void stop();

private:
    int port_;
    bool running_ = false;

    void handleRequest(int clientSocket);
    std::string generateResponse(const std::string& sql);
    std::string generateHtmlResponse();
};

#endif // HTTPSERVER_H
