#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest {
public:
    enum class Method { Get, Post, Head, Unsupported };

    HttpRequest() = default;

    Method method() const;
    const std::string& uri() const;
    const std::unordered_map<std::string, std::string>& headers() const;
    const std::vector<char>& body() const;
    std::string headerValue(const std::string& key) const;

    static bool tryParse(std::vector<char>& buffer, HttpRequest& request);

private:
    Method method_{Method::Unsupported};
    std::string uri_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_REQUEST_H
