#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <vector>

class HttpRequest {
public:
    HttpRequest();
    
    // Parse raw HTTP request data
    bool parse(const std::vector<char>& data);
    
    // Getters
    std::string getMethod() const { return method_; }
    std::string getPath() const { return path_; }
    std::string getHeader(const std::string& name) const;
    const std::vector<char>& getBody() const { return body_; }
    
private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    
    void parseRequestLine(const std::string& line);
    void parseHeader(const std::string& line);
    std::string toLower(const std::string& str) const;
};

#endif // HTTP_REQUEST_H
