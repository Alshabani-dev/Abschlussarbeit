#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest {
public:
    bool parse(const std::vector<char>& rawData);

    const std::string& method() const { return method_; }
    const std::string& path() const { return path_; }
    const std::string& version() const { return version_; }

    std::optional<std::string> header(const std::string& key) const;
    const std::vector<char>& body() const { return body_; }

    static std::optional<std::pair<size_t, size_t>> findHeaderTerminator(const std::vector<char>& buffer);
    static std::optional<size_t> extractContentLength(const std::string& headers);
    static std::string extractMethod(const std::string& requestLine);

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;

    static std::string toLower(std::string value);
};

#endif // HTTP_REQUEST_H
