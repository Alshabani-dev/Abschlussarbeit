#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>
#include <vector>

class HttpResponse {
public:
    HttpResponse();
    void setStatus(int statusCode, const std::string& statusText);
    void addHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void setBody(const std::vector<char>& body);
    std::string toString() const;
    const std::vector<char>& getBinaryBody() const;

private:
    int statusCode_;
    std::string statusText_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    bool isBinary_;
};

#endif // HTTP_RESPONSE_H
