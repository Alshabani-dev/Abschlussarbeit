#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <functional>
#include <map>
#include "Engine.h"

class HttpServer {
public:
    HttpServer(int port, Engine &engine);
    ~HttpServer();

    void start();
    void stop();

private:
    int port_;
    bool running_;
    int serverSocket_;
    Engine &engine_;

    void handleClient(int clientSocket);
    std::string buildResponse(const std::string &content, const std::string &contentType = "text/html", int statusCode = 200);
};

#endif // HTTPSERVER_H
