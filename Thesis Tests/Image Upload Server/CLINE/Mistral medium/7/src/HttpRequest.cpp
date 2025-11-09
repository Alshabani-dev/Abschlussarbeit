#include "HttpRequest.h"
#include <algorithm>
#include <sstream>
#include <iostream>

HttpRequest::HttpRequest(const std::string& rawRequest) {
    parseRequest(rawRequest);
}

void HttpRequest::parseRequest(const std::string& rawRequest) {
    try {
        // Check if request is empty
        if (rawRequest.empty()) {
            throw std::runtime_error("Empty request");
        }

        // Find header end
        size_t headerEnd = rawRequest.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = rawRequest.find("\n\n");
            if (headerEnd == std::string::npos) {
                headerEnd = rawRequest.length();
            } else {
                headerEnd += 2;
            }
        } else {
            headerEnd += 4;
        }

        // Parse request line
        size_t firstLineEnd = rawRequest.find("\r\n");
        if (firstLineEnd == std::string::npos) {
            firstLineEnd = rawRequest.find("\n");
            if (firstLineEnd == std::string::npos) {
                throw std::runtime_error("Invalid request format: no line ending found");
            }
        }

        std::string requestLine = rawRequest.substr(0, firstLineEnd);
        std::istringstream requestStream(requestLine);

        if (!(requestStream >> method_ >> path_ >> version_)) {
            throw std::runtime_error("Invalid request line format");
        }

        // Parse headers
        std::string headerSection = rawRequest.substr(0, headerEnd - (headerEnd == rawRequest.length() ? 0 : 4));
        parseHeaders(headerSection);

        // Parse body
        if (headerEnd < rawRequest.length()) {
            body_ = rawRequest.substr(headerEnd);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing HTTP request: " << e.what() << std::endl;
        method_ = "ERROR";
        path_ = "/error";
        version_ = "HTTP/1.1";
        body_ = "";
    }
}

void HttpRequest::parseHeaders(const std::string& headerSection) {
    std::istringstream headerStream(headerSection);
    std::string line;

    // Skip request line
    std::getline(headerStream, line);

    while (std::getline(headerStream, line)) {
        if (line.empty() || line == "\r") break;

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);

            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

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

std::string HttpRequest::getVersion() const {
    return version_;
}

std::string HttpRequest::getHeader(const std::string& name) const {
    auto it = headers_.find(name);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

bool HttpRequest::hasHeader(const std::string& name) const {
    return headers_.find(name) != headers_.end();
}

std::string HttpRequest::getBody() const {
    return body_;
}

size_t HttpRequest::getContentLength() const {
    if (hasHeader("Content-Length")) {
        return std::stoul(getHeader("Content-Length"));
    }
    return 0;
}
