#include "HttpResponse.h"

#include <cctype>
#include <sstream>

HttpResponse::HttpResponse() = default;

void HttpResponse::setStatus(int code, std::string message) {
    statusCode_ = code;
    statusMessage_ = std::move(message);
}

void HttpResponse::setHeader(std::string key, std::string value) {
    headers_[std::move(key)] = std::move(value);
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
    headers_["Content-Length"] = std::to_string(body_.size());
}

void HttpResponse::setBody(const std::string& body) {
    body_ = std::vector<char>(body.begin(), body.end());
    headers_["Content-Length"] = std::to_string(body_.size());
}

std::vector<char> HttpResponse::serialize() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode_ << ' ' << statusMessage_ << "\r\n";
    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    const std::string head = oss.str();
    std::vector<char> buffer(head.begin(), head.end());
    buffer.insert(buffer.end(), body_.begin(), body_.end());
    return buffer;
}
