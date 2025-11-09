#include "HttpResponse.h"
#include <sstream>
#include <ctime>

HttpResponse::HttpResponse(int statusCode, const std::string& contentType)
    : statusCode_(statusCode), contentType_(contentType) {
    // Set default headers
    setHeader("Content-Type", contentType_);
    setHeader("Connection", "close");
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
    setHeader("Content-Length", std::to_string(body_.size()));
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
    setHeader("Content-Length", std::to_string(body_.size()));
}

std::string HttpResponse::getStatusText() const {
    switch (statusCode_) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown Status";
    }
}

std::vector<char> HttpResponse::toBytes() const {
    std::stringstream responseStream;

    // Status line
    responseStream << "HTTP/1.1 " << statusCode_ << " " << getStatusText() << "\r\n";

    // Headers
    for (const auto& header : headers_) {
        responseStream << header.first << ": " << header.second << "\r\n";
    }

    // End of headers
    responseStream << "\r\n";

    // Convert to vector<char>
    std::string responseStr = responseStream.str();
    std::vector<char> responseBytes(responseStr.begin(), responseStr.end());

    // Append body
    responseBytes.insert(responseBytes.end(), body_.begin(), body_.end());

    return responseBytes;
}
