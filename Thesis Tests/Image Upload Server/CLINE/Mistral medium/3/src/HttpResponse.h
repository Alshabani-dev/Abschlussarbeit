#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include <string>
#include <vector>
#include <map>

class HttpResponse {
public:
    HttpResponse(int statusCode, const std::string& statusText, const std::string& contentType = "text/html");
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);
    std::vector<char> build() const;

private:
    int statusCode_;
    std::string statusText_;
    std::string contentType_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
};
#endif // HTTPRESPONSE_H
