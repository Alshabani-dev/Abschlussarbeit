#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest {
public:
    static HttpRequest parse(const std::vector<char>& data);

    const std::string& method() const { return method_; }
    const std::string& path() const { return path_; }
    const std::string& version() const { return version_; }
    const std::unordered_map<std::string, std::string>& headers() const { return headers_; }
    const std::vector<char>& body() const { return body_; }

    bool headerEquals(const std::string& key, const std::string& value) const;
    std::string headerValue(const std::string& key) const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_REQUEST_H
