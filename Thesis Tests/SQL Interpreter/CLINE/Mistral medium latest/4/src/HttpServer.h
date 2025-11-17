#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>
#include <functional>

class HttpServer {
public:
    HttpServer(int port, const std::function<std::string(const std::string&)>& sqlExecutor);
    ~HttpServer();

    void start();
    void stop();

private:
    int port_;
    std::function<std::string(const std::string&)> sqlExecutor_;
    bool running_;

    void handleClient(int clientSocket);
    std::string buildResponse(const std::string& request);
    std::string executeSqlCommand(const std::string& sql);
};

#endif // HTTPSERVER_H
