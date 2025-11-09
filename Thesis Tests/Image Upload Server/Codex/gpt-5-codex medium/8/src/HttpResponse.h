#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    HttpResponse();

    void setStatus(int code, std::string message);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& data);
    void setBody(std::vector<char>&& data);
    void setBody(const std::string& data);

    std::vector<char> serialize() const;

private:
    int statusCode_;
    std::string statusMessage_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
