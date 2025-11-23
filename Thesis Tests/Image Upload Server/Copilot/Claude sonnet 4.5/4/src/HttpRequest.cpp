#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cctype>

HttpRequest::HttpRequest() : complete_(false) {}

std::string HttpRequest::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool HttpRequest::parse(const std::vector<char>& rawData) {
    if (rawData.empty()) {
        return false;
    }
    
    // Convert to string for header parsing (up to header/body separator)
    std::string rawStr(rawData.begin(), rawData.end());
    
    // Find header/body separator (try \r\n\r\n first, then \n\n)
    size_t headerEnd = rawStr.find("\r\n\r\n");
    size_t headerLength;
    if (headerEnd != std::string::npos) {
        headerLength = headerEnd + 4;
    } else {
        headerEnd = rawStr.find("\n\n");
        if (headerEnd != std::string::npos) {
            headerLength = headerEnd + 2;
        } else {
            // Headers not complete yet
            return false;
        }
    }
    
    // Extract headers section
    std::string headersSection = rawStr.substr(0, headerEnd);
    std::istringstream stream(headersSection);
    std::string line;
    
    // Parse request line
    if (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        parseRequestLine(line);
    }
    
    // Parse headers
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            parseHeader(line);
        }
    }
    
    // Extract body (binary-safe)
    if (headerLength < rawData.size()) {
        body_.assign(rawData.begin() + headerLength, rawData.end());
    }
    
    // Check if request is complete based on Content-Length
    std::string contentLengthStr = getHeader("content-length");
    if (!contentLengthStr.empty()) {
        size_t contentLength = std::stoull(contentLengthStr);
        complete_ = (body_.size() >= contentLength);
    } else {
        // No Content-Length header (e.g., GET request)
        complete_ = true;
    }
    
    return complete_;
}

void HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream stream(line);
    stream >> method_ >> path_ >> version_;
}

void HttpRequest::parseHeader(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        std::string name = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // Trim leading whitespace from value
        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos) {
            value = value.substr(start);
        }
        
        // Store header with lowercase name for case-insensitive lookup
        headers_[toLower(name)] = value;
    }
}

std::string HttpRequest::getHeader(const std::string& name) const {
    std::string lowerName = toLower(name);
    auto it = headers_.find(lowerName);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::string HttpRequest::getBodyAsString() const {
    return std::string(body_.begin(), body_.end());
}
