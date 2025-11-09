#include "HttpResponse.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace {

bool equalsIgnoreCase(const std::string& lhs, const std::string& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t i = 0; i < lhs.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(lhs[i])) !=
            std::tolower(static_cast<unsigned char>(rhs[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

void HttpResponse::setStatus(int code, std::string reason) {
    statusCode_ = code;
    reasonPhrase_ = std::move(reason);
}

void HttpResponse::setHeader(std::string key, std::string value) {
    headers_[std::move(key)] = std::move(value);
}

void HttpResponse::setBody(std::vector<char> data) {
    body_ = std::move(data);
}

std::vector<char> HttpResponse::serialize() const {
    std::ostringstream headStream;
    headStream << "HTTP/1.1 " << statusCode_ << ' ' << reasonPhrase_ << "\r\n";

    bool hasContentLength = false;
    bool hasConnection = false;

    for (const auto& [key, value] : headers_) {
        if (equalsIgnoreCase(key, "Content-Length")) {
            hasContentLength = true;
        }
        if (equalsIgnoreCase(key, "Connection")) {
            hasConnection = true;
        }
        headStream << key << ": " << value << "\r\n";
    }

    if (!hasContentLength) {
        headStream << "Content-Length: " << body_.size() << "\r\n";
    }

    if (!hasConnection) {
        headStream << "Connection: close\r\n";
    }

    headStream << "\r\n";
    std::string headerBytes = headStream.str();

    std::vector<char> payload;
    payload.reserve(headerBytes.size() + body_.size());
    payload.insert(payload.end(), headerBytes.begin(), headerBytes.end());
    payload.insert(payload.end(), body_.begin(), body_.end());

    return payload;
}

size_t HttpResponse::bodySize() const {
    return body_.size();
}
