#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse(int statusCode) : statusCode_(statusCode) {
    // Set default headers
    headers_["Connection"] = "close";
}

void HttpResponse::setStatus(int statusCode) {
    statusCode_ = statusCode;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpResponse::setBody(const std::string& body) {
    body_ = body;
}

std::string HttpResponse::toString() const {
    std::ostringstream oss;

    // Status line
    oss << "HTTP/1.1 " << statusCode_ << " ";

    switch (statusCode_) {
        case 200: oss << "OK"; break;
        case 400: oss << "Bad Request"; break;
        case 404: oss << "Not Found"; break;
        case 500: oss << "Internal Server Error"; break;
        default: oss << "Unknown Status"; break;
    }

    oss << "\r\n";

    // Headers
    for (const auto& header : headers_) {
        oss << header.first << ": " << header.second << "\r\n";
    }

    // Add Content-Length if not set
    if (headers_.find("Content-Length") == headers_.end()) {
        oss << "Content-Length: " << body_.size() << "\r\n";
    }

    // End of headers
    oss << "\r\n";

    // Body
    oss << body_;

    return oss.str();
}
