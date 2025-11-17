#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include "Engine.h"

class HttpServer {
public:
    explicit HttpServer(int port);
    ~HttpServer();
    
    bool start();
    void stop();
    
private:
    int port_;
    int serverSocket_;
    bool running_;
    Engine engine_;
    
    void handleClient(int clientSocket);
    std::string parseRequest(const std::string& request, std::string& method, std::string& path);
    std::string buildResponse(int statusCode, const std::string& contentType, const std::string& body);
    std::string getIndexHtml();
    std::string urlDecode(const std::string& str);
};

#endif // HTTPSERVER_H
