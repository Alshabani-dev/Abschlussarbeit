#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>

class Engine;

class HttpServer {
public:
    HttpServer(Engine &engine, int port);
    void start();

private:
    void handleClient(int clientSocket);
    std::string buildHttpResponse(const std::string &body,
                                  const std::string &contentType,
                                  const std::string &status = "200 OK");
    std::string handleRequest(const std::string &method,
                              const std::string &path,
                              const std::string &body);

    Engine &engine_;
    int port_;
};

#endif // HTTP_SERVER_H
