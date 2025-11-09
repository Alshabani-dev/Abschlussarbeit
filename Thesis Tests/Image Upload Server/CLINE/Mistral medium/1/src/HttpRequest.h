#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include <string>
#include <vector>
#include <map>

class HttpRequest {
public:
    HttpRequest(const std::vector<char>& rawRequest);
    std::string getMethod() const;
    std::string getPath() const;
    std::string getHeader(const std::string& name) const;
    std::vector<char> getBody() const;
    std::string getContentType() const;
    size_t getContentLength() const;

private:
    std::string method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;

    void parseRequest(const std::vector<char>& rawRequest);
};
#endif // HTTPREQUEST_H
