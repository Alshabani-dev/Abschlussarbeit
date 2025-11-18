#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include "Engine.h"

class HttpServer {
public:
    explicit HttpServer(int port = 8080);
    ~HttpServer();
    
    void start();
    void stop();

private:
    int port_;
    int serverSocket_;
    bool running_;
    Engine engine_;
    
    void handleClient(int clientSocket);
    std::string parseHttpRequest(const std::string& request);
    std::string createHttpResponse(const std::string& body, const std::string& contentType = "text/html");
    std::string getHtmlPage();
    std::string urlDecode(const std::string& str);
};

#endif // HTTPSERVER_H
