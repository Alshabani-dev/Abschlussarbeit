#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest {
public:
    HttpRequest() = default;

    static bool tryParse(const std::vector<char>& buffer, HttpRequest& request, std::size_t& consumedBytes);

    const std::string& method() const { return method_; }
    const std::string& path() const { return path_; }
    const std::string& version() const { return version_; }
    const std::vector<char>& body() const { return body_; }

    bool hasHeader(const std::string& key) const;
    std::string headerValue(const std::string& key) const;
    const std::unordered_map<std::string, std::string>& headers() const { return headers_; }

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;

    static std::string toLower(const std::string& value);
};

#endif // HTTP_REQUEST_H
