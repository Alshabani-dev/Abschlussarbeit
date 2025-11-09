#include "HttpRequest.h"
#include <algorithm>
#include <sstream>
#include <vector>

HttpRequest::HttpRequest() {}

void HttpRequest::parse(const std::string& rawRequest) {
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = rawRequest.find("\n\n");
        if (headerEnd == std::string::npos) {
            throw std::runtime_error("Invalid HTTP request: no header/body separator");
        }
    }

    // Parse request line
    size_t firstLineEnd = rawRequest.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        firstLineEnd = rawRequest.find("\n");
        if (firstLineEnd == std::string::npos) {
            throw std::runtime_error("Invalid HTTP request: no request line");
        }
    }

    std::string requestLine = rawRequest.substr(0, firstLineEnd);
    std::istringstream requestLineStream(requestLine);
    requestLineStream >> method_ >> path_;

    // Parse headers
    size_t headerStart = firstLineEnd + (rawRequest[firstLineEnd] == '\r' ? 2 : 1);
    size_t headerBlockEnd = headerEnd;
    std::string headersBlock = rawRequest.substr(headerStart, headerBlockEnd - headerStart);

    std::istringstream headersStream(headersBlock);
    std::string headerLine;

    while (std::getline(headersStream, headerLine)) {
        // Remove trailing \r if present
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }

        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string name = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);

            // Trim whitespace
            name.erase(name.begin(), std::find_if(name.begin(), name.end(), [](int ch) { return !std::isspace(ch); }));
            name.erase(std::find_if(name.rbegin(), name.rend(), [](int ch) { return !std::isspace(ch); }).base(), name.end());

            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](int ch) { return !std::isspace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [](int ch) { return !std::isspace(ch); }).base(), value.end());

            headers_[name] = value;
        }
    }

    // Extract body
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

std::map<std::string, std::string> HttpRequest::getHeaders() const {
    return headers_;
}
