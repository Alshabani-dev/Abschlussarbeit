#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <vector>
#include <map>

class HttpResponse {
public:
    HttpResponse(int status_code = 200);
    ~HttpResponse();

    void setStatus(int status_code);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void setBody(const std::vector<char>& body);

    std::string toString() const;
    const std::vector<char>& getBinaryBody() const;

private:
    int status_code_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    bool binary_body_;

    std::string getStatusText() const;
    std::string getDefaultHeader(const std::string& name) const;
};

#endif // HTTP_RESPONSE_H
