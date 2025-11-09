#include "HttpRequest.h"
#include <algorithm>
#include <sstream>

HttpRequest::HttpRequest() {}

void HttpRequest::parse(const std::string& rawRequest) {
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = rawRequest.find("\n\n");
    }

    if (headerEnd == std::string::npos) {
        // No body or malformed request
        return;
    }

    // Parse request line
    size_t requestLineEnd = rawRequest.find("\r\n");
    if (requestLineEnd == std::string::npos) {
        requestLineEnd = rawRequest.find("\n");
    }

    if (requestLineEnd != std::string::npos) {
        std::string requestLine = rawRequest.substr(0, requestLineEnd);
        std::istringstream iss(requestLine);
        iss >> method_ >> path_ >> version_;
    }

    // Parse headers
    size_t headerStart = requestLineEnd + (rawRequest[requestLineEnd] == '\r' ? 2 : 1);
    size_t headerBlockEnd = headerEnd;

    std::string headersBlock = rawRequest.substr(headerStart, headerBlockEnd - headerStart);
    std::istringstream headerStream(headersBlock);
    std::string headerLine;

    while (std::getline(headerStream, headerLine)) {
        if (headerLine.empty()) continue;

        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string name = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            headers_[name] = value;
        }
    }

    // Parse body if exists
    if (headerEnd + 4 < rawRequest.size()) {
        body_ = rawRequest.substr(headerEnd + 4);
    }
}

std::string HttpRequest::getMethod() const {
    return method_;
}

std::string HttpRequest::getPath() const {
    return path_;
}

std::string HttpRequest::getHeader(const std::string& name) const {
    auto it = headers_.find(name);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::string HttpRequest::getBody() const {
    return body_;
}

size_t HttpRequest::getContentLength() const {
    std::string contentLength = getHeader("Content-Length");
    if (!contentLength.empty()) {
        return std::stoul(contentLength);
    }
    return 0;
}
