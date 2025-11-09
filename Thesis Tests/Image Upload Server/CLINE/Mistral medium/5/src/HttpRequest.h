#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <vector>
#include <map>

class HttpRequest {
public:
    HttpRequest(const std::string& raw_request);
    ~HttpRequest();

    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::string& getVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::vector<char>& getBody() const;
    size_t getContentLength() const;

    bool isValid() const;
    std::string getHeader(const std::string& name) const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::vector<char> body_;
    bool valid_;

    void parseRequestLine(const std::string& line);
    void parseHeaders(const std::string& header_data);
    void parseBody(const std::string& raw_request);
    size_t findHeaderEnd(const std::string& raw_request) const;
    std::string extractHeaderValue(const std::string& header_line) const;
};

#endif // HTTP_REQUEST_H
