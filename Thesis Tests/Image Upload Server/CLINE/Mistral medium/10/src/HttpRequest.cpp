#include "HttpRequest.h"
#include <algorithm>
#include <sstream>
#include <vector>

HttpRequest::HttpRequest() : contentLength_(0) {}

void HttpRequest::parse(const std::string& rawRequest) {
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = rawRequest.find("\n\n");
        if (headerEnd == std::string::npos) {
            return; // Invalid request format
        }
        headerEnd += 1;
    } else {
        headerEnd += 3;
    }

    // Parse request line and headers
    std::istringstream headerStream(rawRequest.substr(0, headerEnd));
    std::string line;

    // Parse request line
    std::getline(headerStream, line);
    std::istringstream requestLine(line);
    requestLine >> method_ >> path_;

    // Parse headers
    while (std::getline(headerStream, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // Trim whitespace
            name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());
            value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
            headers_[name] = value;
        }
    }

    // Get content length
    auto it = headers_.find("Content-Length");
    if (it != headers_.end()) {
        contentLength_ = std::stoi(it->second);
    }

    // Extract body if present
    if (headerEnd < rawRequest.size()) {
        body_.assign(rawRequest.begin() + headerEnd, rawRequest.end());
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

std::vector<char> HttpRequest::getBody() const {
    return body_;
}

int HttpRequest::getContentLength() const {
    return contentLength_;
}
