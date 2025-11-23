#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include "Engine.h"

class HttpServer {
public:
    HttpServer(Engine* engine, int port = 8080);
    ~HttpServer();
    
    void start();
    void stop();

private:
    Engine* engine_;
    int port_;
    int serverSocket_;
    bool running_;
    
    void handleClient(int clientSocket);
    std::string parseRequest(const std::string& request);
    std::string buildResponse(const std::string& content, const std::string& contentType = "text/html");
    std::string getIndexHtml() const;
    std::string urlDecode(const std::string& str) const;
};

#endif // HTTPSERVER_H
