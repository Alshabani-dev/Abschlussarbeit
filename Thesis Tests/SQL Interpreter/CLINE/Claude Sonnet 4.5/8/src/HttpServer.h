#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <functional>

class HttpServer {
public:
    HttpServer(int port, std::function<std::string(const std::string&)> sqlExecutor);
    ~HttpServer();
    
    void start();
    void stop();

private:
    int port_;
    int serverSocket_;
    bool running_;
    std::function<std::string(const std::string&)> sqlExecutor_;
    
    void handleClient(int clientSocket);
    std::string getHtmlPage();
    std::string urlDecode(const std::string& str);
    std::string htmlEscape(const std::string& str);
};

#endif // HTTPSERVER_H
