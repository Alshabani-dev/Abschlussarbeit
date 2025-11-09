#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest {
public:
    HttpRequest() = default;

    bool parse(const std::vector<char>& rawData);

    const std::string& method() const { return method_; }
    const std::string& path() const { return path_; }
    const std::string& version() const { return version_; }
    const std::vector<char>& body() const { return body_; }

    std::optional<std::string> header(const std::string& key) const;

    static std::optional<std::size_t> findHeaderBoundary(const std::vector<char>& buffer, std::size_t& delimiterLength);

private:
    static std::string toLower(const std::string& value);

    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
};

#endif // HTTP_REQUEST_H
