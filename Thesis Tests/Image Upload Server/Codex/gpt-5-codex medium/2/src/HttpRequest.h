#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest {
public:
    static HttpRequest parse(const std::vector<char>& raw);

    const std::string& method() const noexcept;
    const std::string& path() const noexcept;
    const std::string& version() const noexcept;
    const std::unordered_map<std::string, std::string>& headers() const noexcept;
    const std::vector<char>& body() const noexcept;

    bool hasHeader(const std::string& key) const;
    std::string headerValue(const std::string& key) const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_REQUEST_H
