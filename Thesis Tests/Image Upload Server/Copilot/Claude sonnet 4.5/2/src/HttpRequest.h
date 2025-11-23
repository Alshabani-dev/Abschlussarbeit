#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <vector>

class HttpRequest {
public:
    HttpRequest();
    
    // Parse raw request data (binary-safe)
    bool parse(const std::vector<char>& raw_data);
    
    // Getters
    std::string getMethod() const { return method_; }
    std::string getPath() const { return path_; }
    std::string getHeader(const std::string& key) const;
    std::vector<char> getBody() const { return body_; }
    bool isComplete() const { return is_complete_; }
    size_t getContentLength() const;
    
private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    bool is_complete_;
    
    void parseRequestLine(const std::string& line);
    void parseHeaders(const std::string& header_section);
    std::string toLower(const std::string& str) const;
};

#endif // HTTP_REQUEST_H
