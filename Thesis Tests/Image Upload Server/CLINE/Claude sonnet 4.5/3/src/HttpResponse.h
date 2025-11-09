#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>
#include <vector>

class HttpResponse {
public:
    HttpResponse();
    
    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);
    
    std::vector<char> build() const;
    
    // Helper methods
    static std::string getMimeType(const std::string& path);
    
private:
    int statusCode_;
    std::string statusMessage_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
