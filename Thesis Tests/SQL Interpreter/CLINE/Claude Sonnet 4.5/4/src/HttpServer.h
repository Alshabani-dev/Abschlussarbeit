#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <functional>

class HttpServer {
public:
    explicit HttpServer(int port);
    ~HttpServer();
    
    void setRequestHandler(std::function<std::string(const std::string&)> handler);
    void start();
    void stop();

private:
    int port_;
    int serverSocket_;
    bool running_;
    std::function<std::string(const std::string&)> requestHandler_;
    
    void handleClient(int clientSocket);
    std::string parsePostData(const std::string& request);
    std::string buildHttpResponse(const std::string& content, const std::string& contentType);
    std::string getIndexHtml();
};

#endif // HTTPSERVER_H
