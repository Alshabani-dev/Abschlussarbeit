#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <vector>
#include <unordered_map>

class HttpResponse {
public:
    HttpResponse();
    
    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& body);
    
    std::vector<char> build() const;

private:
    int statusCode_;
    std::string statusMessage_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
