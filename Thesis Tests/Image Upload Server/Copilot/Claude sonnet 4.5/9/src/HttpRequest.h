#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <vector>
#include <unordered_map>

class HttpRequest {
public:
    HttpRequest(const std::vector<char>& requestData);
    
    std::string getMethod() const { return method_; }
    std::string getPath() const { return path_; }
    std::string getHeader(const std::string& key) const;
    std::vector<char> getBody() const { return body_; }

private:
    std::string method_;
    std::string path_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
    
    void parse(const std::vector<char>& requestData);
    std::string toLowerCase(const std::string& str) const;
};

#endif // HTTP_REQUEST_H
