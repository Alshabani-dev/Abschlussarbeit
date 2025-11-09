#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse(int statusCode, const std::string& statusText, const std::string& contentType, const std::string& body)
    : statusCode_(statusCode), statusText_(statusText), contentType_(contentType), body_(body) {}

std::string HttpResponse::toString() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode_ << " " << statusText_ << "\r\n";
    oss << "Content-Type: " << contentType_ << "\r\n";
    oss << "Content-Length: " << body_.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body_;
    return oss.str();
}
