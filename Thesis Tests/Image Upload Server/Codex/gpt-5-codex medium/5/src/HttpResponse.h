#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    void setStatus(int code, std::string reason = {});
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);

    std::vector<char> serialize() const;

private:
    int statusCode_{200};
    std::string reasonPhrase_{"OK"};
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;

    static std::string defaultReason(int statusCode);
};

#endif // HTTP_RESPONSE_H
