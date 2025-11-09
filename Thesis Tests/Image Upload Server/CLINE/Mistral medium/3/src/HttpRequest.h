#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include <string>
#include <vector>
#include <map>

class HttpRequest {
public:
    HttpRequest();
    void parse(const std::vector<char>& data);
    std::string getMethod() const;
    std::string getPath() const;
    std::string getHeader(const std::string& key) const;
    const std::vector<char>& getBody() const;
    bool hasBody() const;

private:
    std::string method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    bool hasBody_;
};
#endif // HTTPREQUEST_H
