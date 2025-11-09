#include "HttpRequest.h"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest() : hasBody_(false) {}

void HttpRequest::parse(const std::vector<char>& data) {
    // Convert to string for easier parsing
    std::string requestStr(data.begin(), data.end());

    // Find the end of headers
    size_t headerEnd = requestStr.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = requestStr.find("\n\n");
        if (headerEnd == std::string::npos) {
            throw std::runtime_error("Invalid HTTP request: no header/body separator");
        }
    }

    // Parse request line
    size_t firstLineEnd = requestStr.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        firstLineEnd = requestStr.find("\n");
        if (firstLineEnd == std::string::npos) {
            throw std::runtime_error("Invalid HTTP request: no request line");
        }
    }

    std::string requestLine = requestStr.substr(0, firstLineEnd);
    std::istringstream requestLineStream(requestLine);
    requestLineStream >> method_ >> path_;

    // Parse headers
    size_t headerStart = firstLineEnd + (requestStr[firstLineEnd + 1] == '\n' ? 2 : 1);
    size_t headerBlockEnd = headerEnd;
    std::string headersBlock = requestStr.substr(headerStart, headerBlockEnd - headerStart);

    std::istringstream headersStream(headersBlock);
    std::string headerLine;
    while (std::getline(headersStream, headerLine)) {
        // Remove any trailing \r
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }

        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string key = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            headers_[key] = value;
        }
    }

    // Parse body if present
    if (headerEnd + (requestStr[headerEnd + 2] == '\r' ? 4 : 2) < requestStr.size()) {
        hasBody_ = true;
        size_t bodyStart = headerEnd + (requestStr[headerEnd + 2] == '\r' ? 4 : 2);
        body_.assign(requestStr.begin() + bodyStart, requestStr.end());
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

const std::vector<char>& HttpRequest::getBody() const {
    return body_;
}

bool HttpRequest::hasBody() const {
    return hasBody_;
}
