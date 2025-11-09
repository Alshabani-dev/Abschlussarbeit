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
    const std::vector<char>& getBody() const;
    std::map<std::string, std::string> getHeaders() const;

private:
    std::string method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_; // Use vector<char> for binary safety
};

#endif // HTTPREQUEST_H
