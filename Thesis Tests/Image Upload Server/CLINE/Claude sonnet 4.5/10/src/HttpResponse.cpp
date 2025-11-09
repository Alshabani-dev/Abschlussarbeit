#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : statusCode_(200), statusMessage_("OK") {}

void HttpResponse::setStatus(int code, const std::string& message) {
    statusCode_ = code;
    statusMessage_ = message;
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
}

std::vector<char> HttpResponse::build() const {
    std::ostringstream stream;
    
    // Status line
    stream << "HTTP/1.1 " << statusCode_ << " " << statusMessage_ << "\r\n";
    
    // Headers
    for (const auto& header : headers_) {
        stream << header.first << ": " << header.second << "\r\n";
    }
    
    // Content-Length header
    stream << "Content-Length: " << body_.size() << "\r\n";
    
    // End of headers
    stream << "\r\n";
    
    // Convert headers to vector<char>
    std::string headerStr = stream.str();
    std::vector<char> response(headerStr.begin(), headerStr.end());
    
    // Append body (binary-safe)
    response.insert(response.end(), body_.begin(), body_.end());
    
    return response;
}
