#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include <string>
#include <vector>
#include <map>

class HttpResponse {
public:
    HttpResponse(int statusCode, const std::string& contentType = "text/html");
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);
    std::vector<char> toBytes() const;

private:
    int statusCode_;
    std::string contentType_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;

    std::string getStatusText() const;
};
#endif // HTTPRESPONSE_H
