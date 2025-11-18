#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <functional>
#include <string>

class HttpServer {
public:
    using Handler = std::function<std::string(const std::string &)>;

    HttpServer() = default;
    void serve(int port, Handler handler);

private:
    std::string buildHttpResponse(const std::string &body, const std::string &contentType) const;
    std::string renderHtml() const;
};

#endif // HTTP_SERVER_H
