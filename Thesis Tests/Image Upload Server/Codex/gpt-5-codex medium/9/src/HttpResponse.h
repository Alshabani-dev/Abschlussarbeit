#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    void setStatus(int code, std::string reason);
    void setHeader(std::string key, std::string value);
    void setBody(std::vector<char> data);

    std::vector<char> serialize() const;
    size_t bodySize() const;

private:
    int statusCode_{200};
    std::string reasonPhrase_{"OK"};
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
