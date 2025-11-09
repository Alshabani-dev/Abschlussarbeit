#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    HttpResponse();

    void setStatus(int code, std::string message);
    void setHeader(std::string key, std::string value);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);

    const std::vector<char>& body() const { return body_; }
    int statusCode() const { return statusCode_; }
    const std::string& statusMessage() const { return statusMessage_; }

    std::vector<char> serialize() const;

private:
    int statusCode_{200};
    std::string statusMessage_{"OK"};
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
