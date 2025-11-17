#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include "Engine.h"

class HttpServer {
public:
    HttpServer(int port, Engine& engine);
    void start();

private:
    int port_;
    Engine& engine_;

    void handleRequest(int clientSocket);
    std::string buildResponse(const std::string& sql);
};

#endif // HTTPSERVER_H
