#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <vector>

class HttpRequest {
public:
    HttpRequest(const std::vector<char>& rawRequest);
    
    std::string getMethod() const;
    std::string getPath() const;
    std::string getHeader(const std::string& name) const;
    std::vector<char> getBody() const;
    
private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    
    void parse(const std::vector<char>& rawRequest);
    std::string toLower(const std::string& str) const;
};

#endif // HTTP_REQUEST_H
