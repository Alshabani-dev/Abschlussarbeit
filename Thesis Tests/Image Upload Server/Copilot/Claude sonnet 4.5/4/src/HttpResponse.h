#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>
#include <vector>

class HttpResponse {
public:
    HttpResponse();
    
    // Set response properties
    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void setBody(const std::vector<char>& body);
    
    // Build the complete response
    std::vector<char> build() const;
    
    // Helper methods for common responses
    static HttpResponse ok(const std::string& body, const std::string& contentType = "text/html");
    static HttpResponse notFound(const std::string& message = "404 Not Found");
    static HttpResponse badRequest(const std::string& message = "400 Bad Request");
    static HttpResponse internalError(const std::string& message = "500 Internal Server Error");
    
private:
    int statusCode_;
    std::string statusMessage_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
