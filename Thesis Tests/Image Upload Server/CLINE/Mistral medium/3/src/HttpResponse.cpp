#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse(int statusCode, const std::string& statusText, const std::string& contentType)
    : statusCode_(statusCode), statusText_(statusText), contentType_(contentType) {
    // Set default headers
    headers_["Content-Type"] = contentType;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpResponse::setBody(const std::vector<char>& body) {
    body_ = body;
}

void HttpResponse::setBody(const std::string& body) {
    body_.assign(body.begin(), body.end());
}

std::vector<char> HttpResponse::build() const {
    std::vector<char> response;

    // Status line
    std::string statusLine = "HTTP/1.1 " + std::to_string(statusCode_) + " " + statusText_ + "\r\n";
    response.insert(response.end(), statusLine.begin(), statusLine.end());

    // Headers
    for (const auto& header : headers_) {
        // Skip Content-Length header as it will be added later
        if (header.first != "Content-Length") {
            std::string headerLine = header.first + ": " + header.second + "\r\n";
            response.insert(response.end(), headerLine.begin(), headerLine.end());
        }
    }

    // Add Content-Length header
    std::string contentLength = "Content-Length: " + std::to_string(body_.size()) + "\r\n";
    response.insert(response.end(), contentLength.begin(), contentLength.end());

    // Empty line
    response.insert(response.end(), '\r');
    response.insert(response.end(), '\n');

    // Body
    response.insert(response.end(), body_.begin(), body_.end());

    return response;
}
