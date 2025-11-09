#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>
#include <vector>

class HttpResponse {
public:
    HttpResponse(int statusCode = 200);
    void setStatusCode(int statusCode);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void setBody(const std::vector<char>& body);
    std::string toString() const;

private:
    int statusCode_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;

    std::string getStatusText() const;
};

#endif // HTTP_RESPONSE_H
