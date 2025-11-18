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

    std::string buildHtml() const;
};

#endif // HTTP_SERVER_H
