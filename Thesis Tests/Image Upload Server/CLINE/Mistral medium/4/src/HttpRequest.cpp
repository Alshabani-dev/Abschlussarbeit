#include "HttpRequest.h"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest() {}

void HttpRequest::parse(const std::string& rawRequest) {
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = rawRequest.find("\n\n");
        if (headerEnd == std::string::npos) {
            return; // Invalid request format
        }
    }

    std::string headersStr = rawRequest.substr(0, headerEnd);

    // Store body as vector<char> for binary safety
    body_.assign(rawRequest.begin() + headerEnd + 4, rawRequest.end());

    std::istringstream headerStream(headersStr);
    std::string line;

    // Parse request line
    if (std::getline(headerStream, line)) {
        std::istringstream requestLine(line);
        requestLine >> method_ >> path_;
    }

    // Parse headers
    while (std::getline(headerStream, line)) {
        if (line.empty()) continue;

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

const std::vector<char>& HttpRequest::getBody() const {
    return body_;
}

std::map<std::string, std::string> HttpRequest::getHeaders() const {
    return headers_;
}
