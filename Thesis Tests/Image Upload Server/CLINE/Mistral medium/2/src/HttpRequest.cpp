#include "HttpRequest.h"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest(const std::string& rawRequest) {
    parseRequest(rawRequest);
}

void HttpRequest::parseRequest(const std::string& rawRequest) {
    std::istringstream iss(rawRequest);
    std::string line;

    // Parse request line
    std::getline(iss, line);
    size_t methodEnd = line.find(' ');
    if (methodEnd != std::string::npos) {
        method_ = line.substr(0, methodEnd);
        size_t pathEnd = line.find(' ', methodEnd + 1);
        if (pathEnd != std::string::npos) {
            path_ = line.substr(methodEnd + 1, pathEnd - methodEnd - 1);
        }
    }

    // Parse headers
    while (std::getline(iss, line) && line != "\r") {
        if (line.empty()) continue;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string headerName = line.substr(0, colonPos);
            std::string headerValue = line.substr(colonPos + 1);
            // Trim whitespace
            headerName.erase(headerName.find_last_not_of(" \t") + 1);
            headerValue.erase(0, headerValue.find_first_not_of(" \t"));
            headers_.emplace_back(headerName, headerValue);
        }
    }

    // Parse body
    size_t bodyStart = rawRequest.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        body_ = rawRequest.substr(bodyStart + 4);
    }
}

std::string HttpRequest::getMethod() const {
    return method_;
}

std::string HttpRequest::getPath() const {
    return path_;
}

std::string HttpRequest::getBody() const {
    return body_;
}

std::string HttpRequest::getHeader(const std::string& headerName) const {
    for (const auto& header : headers_) {
        if (header.first == headerName) {
            return header.second;
        }
    }
    return "";
}

bool HttpRequest::hasHeader(const std::string& headerName) const {
    for (const auto& header : headers_) {
        if (header.first == headerName) {
            return true;
        }
    }
    return false;
}

size_t HttpRequest::getContentLength() const {
    std::string contentLength = getHeader("Content-Length");
    if (!contentLength.empty()) {
        return std::stoul(contentLength);
    }
    return 0;
}
