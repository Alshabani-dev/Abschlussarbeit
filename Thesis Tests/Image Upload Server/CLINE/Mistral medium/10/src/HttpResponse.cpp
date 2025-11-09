#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : statusCode_(200), statusText_("OK") {}

void HttpResponse::setStatus(int statusCode, const std::string& statusText) {
    statusCode_ = statusCode;
    statusText_ = statusText;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
}

std::string HttpResponse::toString() const {
    std::ostringstream response;

    // Status line
    response << "HTTP/1.1 " << statusCode_ << " " << statusText_ << "\r\n";

    // Headers
    for (const auto& header : headers_) {
        response << header.first << ": " << header.second << "\r\n";
    }

    // Add Content-Length if not already set
    if (headers_.find("Content-Length") == headers_.end()) {
        response << "Content-Length: " << body_.size() << "\r\n";
    }

    // Add Connection: close if not already set
    if (headers_.find("Connection") == headers_.end()) {
        response << "Connection: close\r\n";
    }

    // End of headers
    response << "\r\n";

    // Body
    if (!body_.empty()) {
        response.write(body_.data(), body_.size());
    }

    return response.str();
}
