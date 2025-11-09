#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    HttpResponse(int statusCode = 200, std::string reason = "OK");

    void setStatus(int statusCode, const std::string& reason);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);

    int statusCode() const noexcept;
    const std::string& reason() const noexcept;
    const std::unordered_map<std::string, std::string>& headers() const noexcept;
    const std::vector<char>& body() const noexcept;

    std::vector<char> serialize() const;

private:
    int statusCode_;
    std::string reason_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
