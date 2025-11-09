#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <vector>

class HttpRequest {
public:
    HttpRequest();
    
    // Parse raw request data
    bool parse(const std::vector<char>& data);
    
    // Getters
    std::string getMethod() const { return method_; }
    std::string getPath() const { return path_; }
    std::string getHeader(const std::string& name) const;
    const std::vector<char>& getBody() const { return body_; }
    int getContentLength() const { return contentLength_; }
    
    // Check if request is complete (all data received)
    bool isComplete(const std::vector<char>& buffer) const;
    
private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    int contentLength_;
    
    void parseHeaders(const std::string& headerSection);
    std::string toLowerCase(const std::string& str) const;
};

#endif // HTTP_REQUEST_H
