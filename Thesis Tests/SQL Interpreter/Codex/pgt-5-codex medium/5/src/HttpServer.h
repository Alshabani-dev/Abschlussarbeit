#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <functional>
#include <string>

class HttpServer {
public:
    explicit HttpServer(int port);
    void start(const std::function<std::string(const std::string &)> &handler);

private:
    int port_;

    std::string buildPage(const std::string &result) const;
    std::string handleRequest(const std::string &request, const std::function<std::string(const std::string &)> &handler) const;
};

#endif // HTTP_SERVER_H
