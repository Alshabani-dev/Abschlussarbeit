#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cstring>

HttpRequest::HttpRequest() {}

bool HttpRequest::parse(const std::vector<char>& data) {
    if (data.empty()) return false;
    
    // Convert to string for header parsing (headers are text)
    std::string dataStr(data.begin(), data.end());
    
    // Find header/body separator - try \r\n\r\n first, then \n\n
    size_t headerEnd = dataStr.find("\r\n\r\n");
    size_t headerLength = 4;
    
    if (headerEnd == std::string::npos) {
        headerEnd = dataStr.find("\n\n");
        if (headerEnd == std::string::npos) {
            return false; // Invalid request
        }
        headerLength = 2;
    }
    
    // Parse headers
    std::string headerSection = dataStr.substr(0, headerEnd);
    std::istringstream stream(headerSection);
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
    size_t bodyStart = headerEnd + headerLength;
    if (bodyStart < data.size()) {
        body_.assign(data.begin() + bodyStart, data.end());
    }
    
    return true;
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
        
        // Store with lowercase key for case-insensitive lookup
        headers_[toLower(name)] = value;
    }
}

std::string HttpRequest::getHeader(const std::string& name) const {
    auto it = headers_.find(toLower(name));
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::string HttpRequest::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
