#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include <string>
#include <vector>
#include <map>

class HttpRequest {
public:
    HttpRequest();
    void parse(const std::string& rawRequest);
    std::string getMethod() const;
    std::string getPath() const;
    std::string getHeader(const std::string& name) const;
    std::string getBody() const;
    std::map<std::string, std::string> getHeaders() const;

private:
    std::string method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};
#endif // HTTPREQUEST_H
