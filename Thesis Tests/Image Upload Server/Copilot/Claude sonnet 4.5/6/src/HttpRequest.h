#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <map>
#include <vector>

class HttpRequest {
public:
    HttpRequest();
    bool parse(const std::vector<char>& rawData);
    
    std::string getMethod() const { return method_; }
    std::string getPath() const { return path_; }
    std::string getHeader(const std::string& name) const;
    const std::vector<char>& getBody() const { return body_; }
    
    bool isComplete(const std::vector<char>& data) const;
    
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

#endif // HTTPREQUEST_H
