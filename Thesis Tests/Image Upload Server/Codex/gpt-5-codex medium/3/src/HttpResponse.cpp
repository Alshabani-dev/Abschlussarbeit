#include "HttpResponse.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

}  // namespace

HttpResponse::HttpResponse() : statusCode_(200), reasonPhrase_("OK") {}

HttpResponse::HttpResponse(int statusCode, std::string reasonPhrase)
    : statusCode_(statusCode), reasonPhrase_(std::move(reasonPhrase)) {}

void HttpResponse::setStatus(int statusCode, const std::string& reasonPhrase) {
    statusCode_ = statusCode;
    reasonPhrase_ = reasonPhrase;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::vector<char>& body, const std::string& contentType) {
    body_ = body;
    setHeader("Content-Type", contentType);
}

void HttpResponse::setBody(std::vector<char>&& body, const std::string& contentType) {
    body_ = std::move(body);
    setHeader("Content-Type", contentType);
}

std::vector<char> HttpResponse::serialize() const {
    std::vector<char> serialized;
    std::string statusLine = "HTTP/1.1 " + std::to_string(statusCode_) + " " + reasonPhrase_ + "\r\n";
    serialized.insert(serialized.end(), statusLine.begin(), statusLine.end());

    bool connectionHeaderPresent = false;
    for (const auto& [key, value] : headers_) {
        std::string lowerKey = toLower(key);
        if (lowerKey == "content-length") {
            continue;  // will set explicitly
        }
        if (lowerKey == "connection") {
            connectionHeaderPresent = true;
        }
        std::string headerLine = key + ": " + value + "\r\n";
        serialized.insert(serialized.end(), headerLine.begin(), headerLine.end());
    }

    if (!connectionHeaderPresent) {
        std::string headerLine = "Connection: close\r\n";
        serialized.insert(serialized.end(), headerLine.begin(), headerLine.end());
    }

    std::string contentLengthLine = "Content-Length: " + std::to_string(body_.size()) + "\r\n";
    serialized.insert(serialized.end(), contentLengthLine.begin(), contentLengthLine.end());

    serialized.push_back('\r');
    serialized.push_back('\n');

    serialized.insert(serialized.end(), body_.begin(), body_.end());
    return serialized;
}
