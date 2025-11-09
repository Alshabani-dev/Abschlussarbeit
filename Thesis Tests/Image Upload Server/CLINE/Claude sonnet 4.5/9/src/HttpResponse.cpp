#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : statusCode_(200), statusMessage_("OK") {}

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

std::vector<char> HttpResponse::build() const {
    std::ostringstream oss;
    
    // Status line
    oss << "HTTP/1.1 " << statusCode_ << " " << statusMessage_ << "\r\n";
    
    // Headers
    for (const auto& header : headers_) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    // End of headers
    oss << "\r\n";
    
    // Convert to vector
    std::string headerStr = oss.str();
    std::vector<char> response(headerStr.begin(), headerStr.end());
    
    // Append body (binary-safe)
    response.insert(response.end(), body_.begin(), body_.end());
    
    return response;
}
