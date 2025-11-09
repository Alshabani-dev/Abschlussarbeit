#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cstring>

HttpRequest::HttpRequest() {}

std::string HttpRequest::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool HttpRequest::isComplete(const std::vector<char>& data) const {
    // Find header end - support both \r\n\r\n and \n\n
    std::string dataStr(data.begin(), data.end());
    size_t headerEnd = dataStr.find("\r\n\r\n");
    size_t headerLength = 4;
    
    if (headerEnd == std::string::npos) {
        headerEnd = dataStr.find("\n\n");
        headerLength = 2;
        if (headerEnd == std::string::npos) {
            return false; // Headers not complete
        }
    }
    
    // Extract headers to check Content-Length
    std::string headers = dataStr.substr(0, headerEnd);
    std::istringstream stream(headers);
    std::string line;
    int contentLength = 0;
    bool hasContentLength = false;
    
    while (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim leading spaces from value
            size_t start = value.find_first_not_of(" \t");
            if (start != std::string::npos) {
                value = value.substr(start);
            }
            
            if (toLower(name) == "content-length") {
                contentLength = std::stoi(value);
                hasContentLength = true;
                break;
            }
        }
    }
    
    if (!hasContentLength) {
        return true; // No body expected
    }
    
    size_t bodyStart = headerEnd + headerLength;
    size_t currentBodySize = data.size() - bodyStart;
    
    return currentBodySize >= static_cast<size_t>(contentLength);
}

bool HttpRequest::parse(const std::vector<char>& rawData) {
    std::string dataStr(rawData.begin(), rawData.end());
    
    // Find header end - support both \r\n\r\n and \n\n
    size_t headerEnd = dataStr.find("\r\n\r\n");
    size_t headerLength = 4;
    
    if (headerEnd == std::string::npos) {
        headerEnd = dataStr.find("\n\n");
        headerLength = 2;
        if (headerEnd == std::string::npos) {
            return false;
        }
    }
    
    // Parse headers
    std::string headers = dataStr.substr(0, headerEnd);
    std::istringstream stream(headers);
    std::string line;
    
    // Parse request line
    if (!std::getline(stream, line)) {
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    parseRequestLine(line);
    
    // Parse header fields
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
    if (bodyStart < rawData.size()) {
        body_.assign(rawData.begin() + bodyStart, rawData.end());
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
        
        // Trim leading spaces from value
        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos) {
            value = value.substr(start);
        }
        
        // Store with lowercase name for case-insensitive lookup
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
