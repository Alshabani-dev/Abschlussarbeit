#include "HttpResponse.h"

#include <sstream>
#include <string>
#include <vector>

HttpResponse::HttpResponse()
    : statusCode_(200),
      statusMessage_("OK") {}

void HttpResponse::setStatus(int code, const std::string& message) {
    statusCode_ = code;
    statusMessage_ = message;
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

std::vector<char> HttpResponse::serialize() const {
    std::ostringstream headerStream;
    headerStream << "HTTP/1.1 " << statusCode_ << ' ' << statusMessage_ << "\r\n";

    auto headersCopy = headers_;
    if (!headersCopy.count("Content-Length") && !headersCopy.count("content-length")) {
        headersCopy["Content-Length"] = std::to_string(body_.size());
    }
    if (!headersCopy.count("Connection") && !headersCopy.count("connection")) {
        headersCopy["Connection"] = "close";
    }

    for (const auto& [key, value] : headersCopy) {
        headerStream << key << ": " << value << "\r\n";
    }
    headerStream << "\r\n";

    const std::string headerString = headerStream.str();
    std::vector<char> response;
    response.reserve(headerString.size() + body_.size());
    response.insert(response.end(), headerString.begin(), headerString.end());
    response.insert(response.end(), body_.begin(), body_.end());
    return response;
}
