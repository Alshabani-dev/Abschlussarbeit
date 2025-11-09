#include "HttpResponse.h"

#include <sstream>

HttpResponse::HttpResponse(int statusCode, std::string statusMessage)
    : statusCode_(statusCode), statusMessage_(std::move(statusMessage)) {
    headers_["Connection"] = "close";
}

void HttpResponse::setStatus(int statusCode, const std::string& statusMessage) {
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::vector<char>& data, const std::string& contentType) {
    body_ = data;
    headers_["Content-Type"] = contentType;
}

void HttpResponse::setBody(const std::string& data, const std::string& contentType) {
    body_.assign(data.begin(), data.end());
    headers_["Content-Type"] = contentType;
}

std::vector<char> HttpResponse::serialize() const {
    std::ostringstream headStream;
    headStream << "HTTP/1.1 " << statusCode_ << ' ' << statusMessage_ << "\r\n";

    headStream << "Content-Length: " << body_.size() << "\r\n";

    for (const auto& [key, value] : headers_) {
        if (key == "Content-Length") {
            // Skip if already handled explicitly.
            continue;
        }
        headStream << key << ": " << value << "\r\n";
    }

    headStream << "\r\n";

    const std::string headString = headStream.str();

    std::vector<char> serialized;
    serialized.reserve(headString.size() + body_.size());
    serialized.insert(serialized.end(), headString.begin(), headString.end());
    serialized.insert(serialized.end(), body_.begin(), body_.end());

    return serialized;
}
