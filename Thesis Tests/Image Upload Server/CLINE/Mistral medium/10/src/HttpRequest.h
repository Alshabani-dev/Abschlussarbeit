#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

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
    std::vector<char> getBody() const;
    int getContentLength() const;

private:
    std::string method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    int contentLength_;
};

#endif // HTTP_REQUEST_H
