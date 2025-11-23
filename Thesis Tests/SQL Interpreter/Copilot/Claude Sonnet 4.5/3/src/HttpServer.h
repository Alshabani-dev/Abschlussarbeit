#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include "Engine.h"

class HttpServer {
public:
    HttpServer(Engine* engine, int port);
    void start();
    
private:
    Engine* engine_;
    int port_;
    int serverSocket_;
    
    void handleClient(int clientSocket);
    std::string parseHttpRequest(const std::string& request, std::string& method, std::string& path);
    std::string createHttpResponse(const std::string& content, const std::string& contentType);
    std::string getIndexHtml();
    std::string urlDecode(const std::string& str);
};

#endif // HTTPSERVER_H
