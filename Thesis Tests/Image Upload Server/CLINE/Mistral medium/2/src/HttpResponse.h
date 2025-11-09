#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include <string>
#include <vector>

class HttpResponse {
public:
    HttpResponse(int statusCode, const std::string& statusText, const std::string& contentType, const std::string& body);
    std::string toString() const;

private:
    int statusCode_;
    std::string statusText_;
    std::string contentType_;
    std::string body_;
};
#endif // HTTP_RESPONSE_H
