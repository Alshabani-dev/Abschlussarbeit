#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : statusCode_(200) {
    statusMessage_ = getStatusMessage(200);
}

void HttpResponse::setStatusCode(int code) {
    statusCode_ = code;
    statusMessage_ = getStatusMessage(code);
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
    
    // Empty line separating headers from body
    responseStream << "\r\n";
    
    // Convert header string to vector
    std::string headerStr = responseStream.str();
    std::vector<char> response(headerStr.begin(), headerStr.end());
    
    // Append body (binary-safe)
    response.insert(response.end(), body_.begin(), body_.end());
    
    return response;
}

HttpResponse HttpResponse::ok(const std::vector<char>& body, const std::string& contentType) {
    HttpResponse response;
    response.setStatusCode(200);
    response.setHeader("Content-Type", contentType);
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::ok(const std::string& body, const std::string& contentType) {
    HttpResponse response;
    response.setStatusCode(200);
    response.setHeader("Content-Type", contentType);
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::notFound(const std::string& message) {
    HttpResponse response;
    response.setStatusCode(404);
    response.setHeader("Content-Type", "text/plain");
    response.setBody(message);
    return response;
}

HttpResponse HttpResponse::badRequest(const std::string& message) {
    HttpResponse response;
    response.setStatusCode(400);
    response.setHeader("Content-Type", "text/plain");
    response.setBody(message);
    return response;
}

HttpResponse HttpResponse::internalError(const std::string& message) {
    HttpResponse response;
    response.setStatusCode(500);
    response.setHeader("Content-Type", "text/plain");
    response.setBody(message);
    return response;
}

std::string HttpResponse::getStatusMessage(int code) const {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}
