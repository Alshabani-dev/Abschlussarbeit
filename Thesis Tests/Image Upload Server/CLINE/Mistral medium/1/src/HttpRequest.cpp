#include "HttpRequest.h"
#include <algorithm>
#include <sstream>
#include <vector>

HttpRequest::HttpRequest(const std::vector<char>& rawRequest) {
    parseRequest(rawRequest);
}

void HttpRequest::parseRequest(const std::vector<char>& rawRequest) {
    // Find the end of headers (first empty line)
    size_t headerEnd = 0;
    for (; headerEnd + 3 < rawRequest.size(); headerEnd++) {
        if (rawRequest[headerEnd] == '\r' && rawRequest[headerEnd+1] == '\n' &&
            rawRequest[headerEnd+2] == '\r' && rawRequest[headerEnd+3] == '\n') {
            headerEnd += 4;
            break;
        } else if (rawRequest[headerEnd] == '\n' && rawRequest[headerEnd+1] == '\n') {
            headerEnd += 2;
            break;
        }
    }

    // Parse request line
    std::string headerStr(rawRequest.begin(), rawRequest.begin() + headerEnd - (rawRequest[headerEnd-4] == '\r' ? 4 : 2));
    std::istringstream headerStream(headerStr);

    // Extract method, path, and HTTP version
    std::string requestLine;
    std::getline(headerStream, requestLine);
    std::istringstream requestLineStream(requestLine);
    requestLineStream >> method_ >> path_;

    // Parse headers
    std::string headerLine;
    while (std::getline(headerStream, headerLine) && !headerLine.empty()) {
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

    // Extract body if present
    auto contentLengthIt = headers_.find("Content-Length");
    if (contentLengthIt != headers_.end()) {
        size_t contentLength = std::stoul(contentLengthIt->second);
        if (headerEnd + contentLength <= rawRequest.size()) {
            body_.assign(rawRequest.begin() + headerEnd, rawRequest.begin() + headerEnd + contentLength);
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

std::vector<char> HttpRequest::getBody() const {
    return body_;
}

std::string HttpRequest::getContentType() const {
    return getHeader("Content-Type");
}

size_t HttpRequest::getContentLength() const {
    std::string lengthStr = getHeader("Content-Length");
    if (!lengthStr.empty()) {
        return std::stoul(lengthStr);
    }
    return 0;
}
