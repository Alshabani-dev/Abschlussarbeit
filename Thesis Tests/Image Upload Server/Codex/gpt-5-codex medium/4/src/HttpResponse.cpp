#include "HttpResponse.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
std::string sizeToString(std::size_t value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
} // namespace

HttpResponse::HttpResponse()
    : statusCode_(200), reasonPhrase_("OK"), body_() {
    headers_["connection"] = "close";
    headers_["content-length"] = "0";
    headers_["content-type"] = "text/plain; charset=utf-8";
}

void HttpResponse::setStatus(int code, std::string reason) {
    statusCode_ = code;
    reasonPhrase_ = std::move(reason);
}

void HttpResponse::setHeader(std::string key, std::string value) {
    headers_[toLower(std::move(key))] = std::move(value);
}

void HttpResponse::setBody(const std::vector<char>& body, const std::string& contentType) {
    body_ = body;
    headers_["content-type"] = contentType;
    headers_["content-length"] = sizeToString(body_.size());
}

void HttpResponse::setBodyText(const std::string& text, const std::string& contentType) {
    std::vector<char> data(text.begin(), text.end());
    setBody(data, contentType);
}

std::vector<char> HttpResponse::serialize() const {
    std::ostringstream head;
    head << "HTTP/1.1 " << statusCode_ << ' ' << reasonPhrase_ << "\r\n";
    for (const auto& [key, value] : headers_) {
        head << key << ": " << value << "\r\n";
    }
    head << "\r\n";

    const std::string headerString = head.str();
    std::vector<char> payload(headerString.begin(), headerString.end());
    payload.insert(payload.end(), body_.begin(), body_.end());
    return payload;
}

std::string HttpResponse::toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}
