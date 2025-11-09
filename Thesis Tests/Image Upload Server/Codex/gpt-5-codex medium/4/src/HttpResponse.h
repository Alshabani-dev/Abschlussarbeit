#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    HttpResponse();

    void setStatus(int code, std::string reason);
    void setHeader(std::string key, std::string value);
    void setBody(const std::vector<char>& body, const std::string& contentType);
    void setBodyText(const std::string& text, const std::string& contentType);

    std::vector<char> serialize() const;

private:
    int statusCode_;
    std::string reasonPhrase_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;

    static std::string toLower(std::string value);
};

#endif // HTTP_RESPONSE_H
