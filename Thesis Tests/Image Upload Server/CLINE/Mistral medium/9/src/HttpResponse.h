#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include <string>
#include <map>

class HttpResponse {
public:
    HttpResponse();
    void setStatus(int status);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    std::string build() const;

private:
    int status_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};
#endif // HTTP_RESPONSE_H
