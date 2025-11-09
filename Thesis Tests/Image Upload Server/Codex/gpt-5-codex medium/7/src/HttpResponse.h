#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <map>
#include <string>
#include <vector>

class HttpResponse {
public:
    HttpResponse(int statusCode = 200, std::string statusMessage = "OK");

    void setStatus(int statusCode, const std::string& statusMessage);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& data, const std::string& contentType);
    void setBody(const std::string& data, const std::string& contentType);

    std::vector<char> serialize() const;

private:
    int statusCode_;
    std::string statusMessage_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_RESPONSE_H
