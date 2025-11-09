#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <string>
#include <vector>

class HttpRequest {
public:
    HttpRequest(const std::string& rawRequest);
    std::string getMethod() const;
    std::string getPath() const;
    std::string getBody() const;
    std::string getHeader(const std::string& headerName) const;
    bool hasHeader(const std::string& headerName) const;
    size_t getContentLength() const;

private:
    std::string method_;
    std::string path_;
    std::string body_;
    std::vector<std::pair<std::string, std::string>> headers_;
    void parseRequest(const std::string& rawRequest);
};
#endif // HTTP_REQUEST_H
