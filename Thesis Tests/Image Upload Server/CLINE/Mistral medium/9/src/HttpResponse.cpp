#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : status_(200) {}

void HttpResponse::setStatus(int status) {
    status_ = status;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    body_ = body;
}

std::string HttpResponse::build() const {
    std::ostringstream response;

    // Status line
    response << "HTTP/1.1 " << status_ << " ";

    // Status text
    switch (status_) {
        case 200: response << "OK"; break;
        case 400: response << "Bad Request"; break;
        case 404: response << "Not Found"; break;
        case 500: response << "Internal Server Error"; break;
        default: response << "Unknown Status"; break;
    }

    response << "\r\n";

    // Headers
    for (const auto& header : headers_) {
        response << header.first << ": " << header.second << "\r\n";
    }

    // Default headers if not set
    if (headers_.find("Content-Length") == headers_.end()) {
        response << "Content-Length: " << body_.length() << "\r\n";
    }

    if (headers_.find("Connection") == headers_.end()) {
        response << "Connection: close\r\n";
    }

    // End of headers
    response << "\r\n";

    // Body
    response << body_;

    return response.str();
}
