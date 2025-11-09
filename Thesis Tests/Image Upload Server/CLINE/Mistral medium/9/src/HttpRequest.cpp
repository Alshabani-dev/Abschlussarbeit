#include "HttpRequest.h"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest() : method_(""), path_("") {}

void HttpRequest::parse(const std::string& request) {
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = request.find("\n\n");
        if (headerEnd == std::string::npos) {
            return; // Invalid request format
        }
    }

    // Parse request line and headers
    std::istringstream headerStream(request.substr(0, headerEnd));
    std::string line;

    // Parse request line
    if (std::getline(headerStream, line)) {
        std::istringstream requestLine(line);
        requestLine >> method_ >> path_;
    }

    // Parse headers
    while (std::getline(headerStream, line) && line != "\r") {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);

            // Trim whitespace
            key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
            value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

            headers_[key] = value;
        }
    }

    // Parse body if present
    if (headerEnd + 4 < request.length()) {
        body_ = request.substr(headerEnd + 4);
    }
}

std::string HttpRequest::getMethod() const {
    return method_;
}

std::string HttpRequest::getPath() const {
    return path_;
}

std::string HttpRequest::getHeader(const std::string& key) const {
    auto it = headers_.find(key);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::string HttpRequest::getBody() const {
    return body_;
}

std::map<std::string, std::string> HttpRequest::getHeaders() const {
    return headers_;
}
