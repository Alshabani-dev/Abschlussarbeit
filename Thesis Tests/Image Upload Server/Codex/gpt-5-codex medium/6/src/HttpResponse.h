#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    HttpResponse();

    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(std::vector<char>&& body);
    void setBody(const std::string& body);

    int statusCode() const { return statusCode_; }
    const std::string& statusMessage() const { return statusMessage_; }
    const std::unordered_map<std::string, std::string>& headers() const { return headers_; }
    const std::vector<char>& body() const { return body_; }

    std::vector<char> serialize() const;

private:
    int statusCode_;
    std::string statusMessage_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
