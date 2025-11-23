#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <vector>
#include <map>

class HttpResponse {
public:
    HttpResponse();
    
    // Set response components
    void setStatusCode(int code);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void setBody(const std::vector<char>& body);
    
    // Build the complete HTTP response
    std::vector<char> build() const;
    
    // Helper methods
    static std::string getMimeType(const std::string& path);
    
private:
    int statusCode_;
    std::string statusMessage_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    
    std::string getStatusMessage(int code) const;
};

#endif // HTTP_RESPONSE_H
