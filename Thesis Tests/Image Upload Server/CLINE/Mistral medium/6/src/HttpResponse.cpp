#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : statusCode_(200), statusText_("OK"), isBinary_(false) {
    // Default headers
    addHeader("Server", "SimpleC++WebServer");
}

void HttpResponse::setStatus(int statusCode, const std::string& statusText) {
    statusCode_ = statusCode;
    statusText_ = statusText;
}

void HttpResponse::addHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
    isBinary_ = false;
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
    isBinary_ = true;
}

std::string HttpResponse::toString() const {
    std::ostringstream oss;

    // Status line
    oss << "HTTP/1.1 " << statusCode_ << " " << statusText_ << "\r\n";

    // Headers
    for (const auto& header : headers_) {
        oss << header.first << ": " << header.second << "\r\n";
    }

    // Add Content-Length if not present
    if (headers_.find("Content-Length") == headers_.end() && !body_.empty()) {
        oss << "Content-Length: " << body_.size() << "\r\n";
    }

    // Add Connection: close if not present
    if (headers_.find("Connection") == headers_.end()) {
        oss << "Connection: close\r\n";
    }

    oss << "\r\n";

    // Body (if not binary)
    if (!isBinary_ && !body_.empty()) {
        oss << std::string(body_.begin(), body_.end());
    }

    return oss.str();
}

const std::vector<char>& HttpResponse::getBinaryBody() const {
    return body_;
}
