#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() 
    : statusCode_(200), statusMessage_("OK") {}

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
    std::ostringstream headerStream;
    
    // Status line
    headerStream << "HTTP/1.1 " << statusCode_ << " " << statusMessage_ << "\r\n";
    
    // Headers
    for (const auto& header : headers_) {
        headerStream << header.first << ": " << header.second << "\r\n";
    }
    
    // Content-Length if not already set
    if (headers_.find("Content-Length") == headers_.end()) {
        headerStream << "Content-Length: " << body_.size() << "\r\n";
    }
    
    // Connection close if not already set
    if (headers_.find("Connection") == headers_.end()) {
        headerStream << "Connection: close\r\n";
    }
    
    // End of headers
    headerStream << "\r\n";
    
    // Build response
    std::string headerStr = headerStream.str();
    std::vector<char> response(headerStr.begin(), headerStr.end());
    response.insert(response.end(), body_.begin(), body_.end());
    
    return response;
}

std::string HttpResponse::getMimeType(const std::string& path) const {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = path.substr(dotPos + 1);
    
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    if (ext == "bmp") return "image/bmp";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "ico") return "image/x-icon";
    if (ext == "txt") return "text/plain";
    
    return "application/octet-stream";
}
