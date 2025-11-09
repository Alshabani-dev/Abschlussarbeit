#include "HttpResponse.h"
#include <sstream>
#include <algorithm>

HttpResponse::HttpResponse() : status_code_(200) {}

void HttpResponse::setStatusCode(int code) {
    status_code_ = code;
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setContentType(const std::string& content_type) {
    headers_["Content-Type"] = content_type;
}

std::vector<char> HttpResponse::build() const {
    std::ostringstream response_stream;
    
    // Status line
    response_stream << "HTTP/1.1 " << status_code_ << " " 
                   << getStatusMessage(status_code_) << "\r\n";
    
    // Headers
    for (const auto& header : headers_) {
        response_stream << header.first << ": " << header.second << "\r\n";
    }
    
    // Content-Length header (always include)
    response_stream << "Content-Length: " << body_.size() << "\r\n";
    
    // Connection header
    response_stream << "Connection: close\r\n";
    
    // Empty line separating headers from body
    response_stream << "\r\n";
    
    // Convert header string to vector
    std::string header_str = response_stream.str();
    std::vector<char> response(header_str.begin(), header_str.end());
    
    // Append body (binary-safe)
    response.insert(response.end(), body_.begin(), body_.end());
    
    return response;
}

std::string HttpResponse::getStatusMessage(int code) const {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

std::string HttpResponse::getMimeType(const std::string& filepath) {
    // Find file extension
    size_t dot_pos = filepath.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = filepath.substr(dot_pos + 1);
    
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    // Map extensions to MIME types
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "bmp") return "image/bmp";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "ico") return "image/x-icon";
    if (ext == "txt") return "text/plain";
    if (ext == "pdf") return "application/pdf";
    
    return "application/octet-stream";
}
