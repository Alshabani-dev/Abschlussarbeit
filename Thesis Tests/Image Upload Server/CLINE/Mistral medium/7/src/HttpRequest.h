#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <vector>
#include <map>

class HttpRequest {
public:
    HttpRequest(const std::string& rawRequest);
    std::string getMethod() const;
    std::string getPath() const;
    std::string getVersion() const;
    std::string getHeader(const std::string& name) const;
    std::string getBody() const;
    bool hasHeader(const std::string& name) const;
    size_t getContentLength() const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::string body_;

    void parseRequest(const std::string& rawRequest);
    void parseHeaders(const std::string& headerSection);
};

#endif // HTTP_REQUEST_H
