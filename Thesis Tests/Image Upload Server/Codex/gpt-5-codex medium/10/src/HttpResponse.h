#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    HttpResponse(int statusCode = 200, std::string reasonPhrase = "OK");

    void setStatus(int code, const std::string& reason);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(std::vector<char>&& body);
    void setBody(const std::string& body);

    std::vector<char> toBytes() const;

private:
    int statusCode_;
    std::string reasonPhrase_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
