#include "HttpResponse.h"

#include <algorithm>
#include <cctype>
#include <sstream>

HttpResponse::HttpResponse(int statusCode, std::string reason)
    : statusCode_(statusCode), reason_(std::move(reason)) {}

void HttpResponse::setStatus(int statusCode, const std::string& reason) {
    statusCode_ = statusCode;
    reason_ = reason;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
}

int HttpResponse::statusCode() const noexcept {
    return statusCode_;
}

const std::string& HttpResponse::reason() const noexcept {
    return reason_;
}

const std::unordered_map<std::string, std::string>& HttpResponse::headers() const noexcept {
    return headers_;
}

const std::vector<char>& HttpResponse::body() const noexcept {
    return body_;
}

std::vector<char> HttpResponse::serialize() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode_ << ' ' << reason_ << "\r\n";

    bool hasContentLength = false;
    bool hasConnection = false;
    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
        std::string loweredKey = key;
        std::transform(loweredKey.begin(), loweredKey.end(), loweredKey.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (loweredKey == "content-length") {
            hasContentLength = true;
        }
        if (loweredKey == "connection") {
            hasConnection = true;
        }
    }

    if (!hasContentLength) {
        oss << "Content-Length: " << body_.size() << "\r\n";
    }
    if (!hasConnection) {
        oss << "Connection: close\r\n";
    }
    oss << "\r\n";

    std::string headerStr = oss.str();
    std::vector<char> buffer(headerStr.begin(), headerStr.end());
    buffer.insert(buffer.end(), body_.begin(), body_.end());
    return buffer;
}
