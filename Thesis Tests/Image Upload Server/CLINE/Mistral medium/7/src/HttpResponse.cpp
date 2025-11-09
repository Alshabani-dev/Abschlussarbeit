#include "HttpResponse.h"
#include <sstream>
#include <ctime>

HttpResponse::HttpResponse(int statusCode) : statusCode_(statusCode) {
    // Set default headers
    setHeader("Server", "SimpleC++WebServer");
    setHeader("Connection", "close");
}

void HttpResponse::setStatusCode(int statusCode) {
    statusCode_ = statusCode;
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

std::string HttpResponse::getStatusText() const {
    switch (statusCode_) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown Status";
    }
}

std::string HttpResponse::toString() const {
    std::ostringstream response;

    // Status line
    response << "HTTP/1.1 " << statusCode_ << " " << getStatusText() << "\r\n";

    // Headers
    for (const auto& header : headers_) {
        response << header.first << ": " << header.second << "\r\n";
    }

    // Add Content-Length if not set
    if (headers_.find("Content-Length") == headers_.end()) {
        response << "Content-Length: " << body_.size() << "\r\n";
    }

    // End of headers
    response << "\r\n";

    // Body
    if (!body_.empty()) {
        response.write(body_.data(), body_.size());
    }

    return response.str();
}
