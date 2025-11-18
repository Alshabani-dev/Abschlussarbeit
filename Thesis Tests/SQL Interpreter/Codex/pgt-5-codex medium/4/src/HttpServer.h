#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>

class Engine;

class HttpServer {
public:
    explicit HttpServer(Engine &engine);
    void run(int port);

private:
    Engine &engine_;

    void handleClient(int clientFd);
    void sendHttpResponse(int clientFd, const std::string &status, const std::string &contentType, const std::string &body);
    std::string buildHtmlPage() const;
};

#endif // HTTP_SERVER_H
