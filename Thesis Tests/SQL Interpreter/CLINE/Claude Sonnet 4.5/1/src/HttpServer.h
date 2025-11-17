#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include "Engine.h"

class HttpServer {
public:
    HttpServer(Engine* engine, int port = 8080);
    ~HttpServer();
    
    bool start();
    void stop();
    
private:
    Engine* engine_;
    int port_;
    int serverSocket_;
    bool running_;
    
    void handleClient(int clientSocket);
    std::string parseHttpRequest(const std::string& request, std::string& method, std::string& path, std::string& body);
    std::string createHttpResponse(int statusCode, const std::string& contentType, const std::string& body);
    std::string getIndexHtml();
    std::string urlDecode(const std::string& str);
};

#endif // HTTPSERVER_H
