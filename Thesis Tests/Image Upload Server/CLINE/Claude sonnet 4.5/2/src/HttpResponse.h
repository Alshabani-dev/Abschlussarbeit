#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>
#include <vector>

class HttpResponse {
public:
    HttpResponse();
    
    // Set response properties
    void setStatusCode(int code);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);
    void setHeader(const std::string& key, const std::string& value);
    void setContentType(const std::string& content_type);
    
    // Build the complete HTTP response (binary-safe)
    std::vector<char> build() const;
    
    // Helper methods
    static std::string getMimeType(const std::string& filepath);
    
private:
    int status_code_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    
    std::string getStatusMessage(int code) const;
};

#endif // HTTP_RESPONSE_H
