#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include <string>
#include <map>

class HttpResponse {
public:
    HttpResponse(int statusCode = 200);
    void setStatus(int statusCode);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    std::string toString() const;

private:
    int statusCode_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};
#endif // HTTPRESPONSE_H
