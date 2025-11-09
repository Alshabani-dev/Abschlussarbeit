#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cstring>

HttpRequest::HttpRequest() : complete_(false) {}

bool HttpRequest::parse(const std::vector<char>& rawData) {
    if (rawData.empty()) {
        return false;
    }
    
    // Find header/body separator - support both \r\n\r\n and \n\n
    size_t headerEnd = std::string::npos;
    size_t headerSize = 0;
    
    // Convert to string for header parsing only
    std::string rawStr(rawData.begin(), rawData.end());
    
    // Try \r\n\r\n first
    headerEnd = rawStr.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        headerSize = headerEnd + 4;
    } else {
        // Try \n\n
        headerEnd = rawStr.find("\n\n");
        if (headerEnd != std::string::npos) {
            headerSize = headerEnd + 2;
        } else {
            // Headers not complete yet
            return false;
        }
    }
    
    // Extract header section
    std::string headerSection = rawStr.substr(0, headerEnd);
    
    // Parse request line
    std::istringstream headerStream(headerSection);
    std::string requestLine;
    std::getline(headerStream, requestLine);
    
    // Remove \r if present
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }
    
    std::istringstream requestLineStream(requestLine);
    requestLineStream >> method_ >> path_ >> version_;
    
    if (method_.empty() || path_.empty()) {
        return false;
    }
    
    // Parse headers
    parseHeaders(headerSection);
    
    // Extract body (binary-safe)
    if (headerSize < rawData.size()) {
        body_.assign(rawData.begin() + headerSize, rawData.end());
    }
    
    // Check if request is complete based on Content-Length
    std::string contentLengthStr = getHeader("content-length");
    if (!contentLengthStr.empty()) {
        size_t contentLength = std::stoull(contentLengthStr);
        if (body_.size() >= contentLength) {
            // Trim body to exact content length
            body_.resize(contentLength);
            complete_ = true;
        } else {
            complete_ = false;
        }
    } else {
        // No body expected (GET/HEAD) or body is complete
        complete_ = true;
    }
    
    return true;
}

void HttpRequest::parseHeaders(const std::string& headerSection) {
    std::istringstream stream(headerSection);
    std::string line;
    
    // Skip request line
    std::getline(stream, line);
    
    // Parse header lines
    while (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) {
            break;
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace from value
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            // Store header with lowercase name for case-insensitive lookup
            headers_[toLowerCase(name)] = value;
        }
    }
}

std::string HttpRequest::getHeader(const std::string& name) const {
    std::string lowerName = toLowerCase(name);
    auto it = headers_.find(lowerName);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::string HttpRequest::toLowerCase(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
