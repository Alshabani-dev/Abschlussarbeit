#include "HttpResponse.h"

#include <algorithm>
#include <cctype>

namespace {
std::string toLower(const std::string& input) {
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}
} // namespace

void HttpResponse::setStatus(int code, std::string reason) {
    statusCode_ = code;
    reasonPhrase_ = reason.empty() ? defaultReason(code) : std::move(reason);
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

std::vector<char> HttpResponse::serialize() const {
    std::string response = "HTTP/1.1 " + std::to_string(statusCode_) + " " + reasonPhrase_ + "\r\n";

    bool hasContentLength = false;
    bool hasConnection = false;

    for (const auto& header : headers_) {
        response += header.first + ": " + header.second + "\r\n";
        std::string lower = toLower(header.first);
        if (lower == "content-length") {
            hasContentLength = true;
        } else if (lower == "connection") {
            hasConnection = true;
        }
    }

    if (!hasContentLength) {
        response += "Content-Length: " + std::to_string(body_.size()) + "\r\n";
    }

    if (!hasConnection) {
        response += "Connection: close\r\n";
    }

    response += "\r\n";

    std::vector<char> bytes(response.begin(), response.end());
    bytes.insert(bytes.end(), body_.begin(), body_.end());
    return bytes;
}

std::string HttpResponse::defaultReason(int statusCode) {
    switch (statusCode) {
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 204:
            return "No Content";
        case 400:
            return "Bad Request";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 411:
            return "Length Required";
        case 413:
            return "Payload Too Large";
        case 415:
            return "Unsupported Media Type";
        case 500:
            return "Internal Server Error";
        default:
            return "Unknown";
    }
}
