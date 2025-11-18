#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>

class Engine;

class HttpServer {
public:
    explicit HttpServer(Engine &engine);
    void start(int port);

private:
    Engine &engine_;
    void handleClient(int clientSocket);
    std::string buildHtmlPage() const;
};

#endif // HTTP_SERVER_H
