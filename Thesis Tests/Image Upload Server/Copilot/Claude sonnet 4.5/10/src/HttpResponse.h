#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <vector>
#include <map>

class HttpResponse {
public:
    HttpResponse();
    
    // Set response properties
    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);
    
    // Build the complete HTTP response
    std::vector<char> build() const;
    
private:
    int statusCode_;
    std::string statusMessage_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
