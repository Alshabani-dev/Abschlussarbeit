#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <functional>
#include <string>

class HttpServer {
public:
    void start(unsigned short port, const std::function<std::string(const std::string &)> &handler);
};

#endif // HTTP_SERVER_H
