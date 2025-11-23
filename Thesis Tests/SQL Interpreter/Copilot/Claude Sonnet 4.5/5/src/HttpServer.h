#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include "Engine.h"

class HttpServer {
public:
    HttpServer(int port, Engine* engine);
    ~HttpServer();
    
    bool start();
    void stop();
    
private:
    int port_;
    int serverSocket_;
    bool running_;
    Engine* engine_;
    
    void handleClient(int clientSocket);
    std::string parseHttpRequest(const std::string& request);
    std::string getHtmlPage();
    std::string createHttpResponse(const std::string& body, const std::string& contentType = "text/html");
};

#endif // HTTPSERVER_H
