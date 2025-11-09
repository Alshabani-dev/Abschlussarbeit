#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : statusCode_(200), statusMessage_("OK") {
}

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

std::vector<char> HttpResponse::build() const {
    std::ostringstream responseStream;
    
    // Status line
    responseStream << "HTTP/1.1 " << statusCode_ << " " << statusMessage_ << "\r\n";
    
    // Add Content-Length header automatically
    responseStream << "Content-Length: " << body_.size() << "\r\n";
    
    // Add Connection: close header to prevent hanging
    responseStream << "Connection: close\r\n";
    
    // Other headers
    for (const auto& header : headers_) {
        responseStream << header.first << ": " << header.second << "\r\n";
    }
    
    // End of headers
    responseStream << "\r\n";
    
    // Convert headers to vector<char>
    std::string headerStr = responseStream.str();
    std::vector<char> response(headerStr.begin(), headerStr.end());
    
    // Append body (binary-safe)
    response.insert(response.end(), body_.begin(), body_.end());
    
    return response;
}
