#include "HttpResponse.h"

#include <algorithm>
#include <sstream>

namespace {

bool equalsIgnoreCase(const std::string& lhs, const std::string& rhs) {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](unsigned char a, unsigned char b) {
               return std::tolower(a) == std::tolower(b);
           });
}

} // namespace

HttpResponse::HttpResponse(int statusCode, std::string reasonPhrase)
    : statusCode_(statusCode), reasonPhrase_(std::move(reasonPhrase)) {}

void HttpResponse::setStatus(int code, const std::string& reason) {
    statusCode_ = code;
    reasonPhrase_ = reason;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
}

void HttpResponse::setBody(std::vector<char>&& body) {
    body_ = std::move(body);
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
}

std::vector<char> HttpResponse::toBytes() const {
    std::ostringstream builder;
    builder << "HTTP/1.1 " << statusCode_ << ' ' << reasonPhrase_ << "\r\n";

    bool hasContentLength = false;
    bool hasConnection = false;

    for (const auto& header : headers_) {
        if (equalsIgnoreCase(header.first, "Content-Length")) {
            hasContentLength = true;
        }
        if (equalsIgnoreCase(header.first, "Connection")) {
            hasConnection = true;
        }
        builder << header.first << ": " << header.second << "\r\n";
    }

    if (!hasContentLength) {
        builder << "Content-Length: " << body_.size() << "\r\n";
    }
    if (!hasConnection) {
        builder << "Connection: close\r\n";
    }

    builder << "\r\n";
    std::string headerString = builder.str();

    std::vector<char> responseBytes;
    responseBytes.reserve(headerString.size() + body_.size());
    responseBytes.insert(responseBytes.end(), headerString.begin(), headerString.end());
    responseBytes.insert(responseBytes.end(), body_.begin(), body_.end());

    return responseBytes;
}
