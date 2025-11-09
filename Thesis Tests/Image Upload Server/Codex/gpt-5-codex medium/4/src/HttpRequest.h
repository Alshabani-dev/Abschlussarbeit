#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest {
public:
    explicit HttpRequest(const std::vector<char>& rawData);

    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::string& getVersion() const;

    bool hasHeader(const std::string& key) const;
    std::string getHeader(const std::string& key) const;

    const std::vector<char>& getBody() const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<char> body_;

    static std::string toLower(std::string value);
    static void trim(std::string& value);
};

#endif // HTTP_REQUEST_H
