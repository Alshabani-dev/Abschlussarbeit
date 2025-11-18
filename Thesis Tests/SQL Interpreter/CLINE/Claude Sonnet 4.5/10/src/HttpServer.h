#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include "Engine.h"

class HttpServer {
public:
    HttpServer(int port, Engine* engine);
    ~HttpServer();
    
    void start();
    void stop();

private:
    int port_;
    int serverSocket_;
    bool running_;
    Engine* engine_;
    
    void handleClient(int clientSocket);
    std::string parseRequest(const std::string& request);
    std::string buildResponse(const std::string& body, const std::string& contentType = "text/html");
    std::string getHtmlPage();
    std::string urlDecode(const std::string& str);
};

#endif // HTTPSERVER_H
