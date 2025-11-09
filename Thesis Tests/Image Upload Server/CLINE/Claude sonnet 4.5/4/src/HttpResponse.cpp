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

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
}

std::vector<char> HttpResponse::build() const {
    std::ostringstream responseStream;
    
    // Status line
    responseStream << "HTTP/1.1 " << statusCode_ << " " << statusMessage_ << "\r\n";
    
    // Headers
    for (const auto& header : headers_) {
        responseStream << header.first << ": " << header.second << "\r\n";
    }
    
    // Content-Length header (always include)
    responseStream << "Content-Length: " << body_.size() << "\r\n";
    
    // Connection header
    responseStream << "Connection: close\r\n";
    
    // Empty line to separate headers from body
    responseStream << "\r\n";
    
    // Convert headers to vector<char>
    std::string headerStr = responseStream.str();
    std::vector<char> response(headerStr.begin(), headerStr.end());
    
    // Append body (binary-safe)
    response.insert(response.end(), body_.begin(), body_.end());
    
    return response;
}

HttpResponse HttpResponse::ok(const std::string& body, const std::string& contentType) {
    HttpResponse response;
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", contentType);
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::notFound(const std::string& message) {
    HttpResponse response;
    response.setStatus(404, "Not Found");
    response.setHeader("Content-Type", "text/plain");
    response.setBody(message);
    return response;
}

HttpResponse HttpResponse::badRequest(const std::string& message) {
    HttpResponse response;
    response.setStatus(400, "Bad Request");
    response.setHeader("Content-Type", "text/plain");
    response.setBody(message);
    return response;
}

HttpResponse HttpResponse::internalError(const std::string& message) {
    HttpResponse response;
    response.setStatus(500, "Internal Server Error");
    response.setHeader("Content-Type", "text/plain");
    response.setBody(message);
    return response;
}
