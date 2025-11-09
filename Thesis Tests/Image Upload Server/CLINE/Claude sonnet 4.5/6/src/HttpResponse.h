#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

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
    
private:
    int statusCode_;
    std::string statusMessage_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    
    std::string getMimeType(const std::string& path) const;
    
    friend class FileHandler;
};

#endif // HTTPRESPONSE_H
