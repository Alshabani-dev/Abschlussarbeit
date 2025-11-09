#include "HttpResponse.h"

#include <sstream>

HttpResponse::HttpResponse()
    : statusCode_(200), statusMessage_("OK") {}

void HttpResponse::setStatus(int code, std::string message) {
    statusCode_ = code;
    statusMessage_ = std::move(message);
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::vector<char>& data) {
    body_ = data;
}

void HttpResponse::setBody(std::vector<char>&& data) {
    body_ = std::move(data);
}

void HttpResponse::setBody(const std::string& data) {
    body_.assign(data.begin(), data.end());
}

std::vector<char> HttpResponse::serialize() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode_ << ' ' << statusMessage_ << "\r\n";
    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    const std::string headerStr = oss.str();

    std::vector<char> result(headerStr.begin(), headerStr.end());
    result.insert(result.end(), body_.begin(), body_.end());
    return result;
}
