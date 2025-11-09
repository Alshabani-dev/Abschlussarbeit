#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include <string>
#include <map>

class HttpResponse {
public:
    HttpResponse();
    void setStatus(int statusCode, const std::string& statusText);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    std::string toString() const;

private:
    int statusCode_;
    std::string statusText_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};
#endif // HTTPRESPONSE_H
