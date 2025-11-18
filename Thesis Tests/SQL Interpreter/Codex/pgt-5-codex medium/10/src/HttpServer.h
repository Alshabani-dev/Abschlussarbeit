#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <functional>
#include <string>

class HttpServer {
public:
    HttpServer();
    void start(unsigned short port, const std::function<std::string(const std::string &)> &handler);

private:
    std::string buildHtmlPage() const;
};

#endif // HTTP_SERVER_H
