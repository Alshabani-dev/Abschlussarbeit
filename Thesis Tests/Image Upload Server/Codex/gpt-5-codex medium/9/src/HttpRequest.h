#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest {
public:
    enum class ParseResult {
        Incomplete,
        Complete,
        Error
    };

    HttpRequest() = default;
    ParseResult parse(const std::vector<char>& rawData);

    const std::string& method() const;
    const std::string& path() const;
    const std::string& version() const;
    const std::unordered_map<std::string, std::string>& headers() const;
    const std::vector<char>& body() const;

    bool isComplete() const;
    const std::string& error() const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;
    bool complete_{false};
    std::string error_;
};

#endif // HTTP_REQUEST_H
