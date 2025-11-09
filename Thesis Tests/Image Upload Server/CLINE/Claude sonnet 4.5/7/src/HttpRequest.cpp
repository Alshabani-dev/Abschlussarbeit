#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cstring>

HttpRequest::HttpRequest() : contentLength_(0) {}

bool HttpRequest::parse(const std::vector<char>& rawData) {
    // Find header end (support both \r\n\r\n and \n\n)
    size_t headerEnd = 0;
    bool found = false;
    
    // Try \r\n\r\n first
    for (size_t i = 0; i < rawData.size() - 3; i++) {
        if (rawData[i] == '\r' && rawData[i+1] == '\n' && 
            rawData[i+2] == '\r' && rawData[i+3] == '\n') {
            headerEnd = i + 4;
            found = true;
            break;
        }
    }
    
    // Try \n\n if not found
    if (!found) {
        for (size_t i = 0; i < rawData.size() - 1; i++) {
            if (rawData[i] == '\n' && rawData[i+1] == '\n') {
                headerEnd = i + 2;
                found = true;
                break;
            }
        }
    }
    
    if (!found) {
        return false;
    }
    
    // Extract headers as string
    std::string headerSection(rawData.begin(), rawData.begin() + headerEnd);
    
    // Parse request line and headers
    std::istringstream stream(headerSection);
    std::string requestLine;
    std::getline(stream, requestLine);
    
    // Remove \r if present
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }
    
    parseRequestLine(requestLine);
    parseHeaders(headerSection);
    
    // Extract body (binary-safe)
    if (headerEnd < rawData.size()) {
        body_.assign(rawData.begin() + headerEnd, rawData.end());
    }
    
    return true;
}

void HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream stream(line);
    stream >> method_ >> path_ >> version_;
}

void HttpRequest::parseHeaders(const std::string& headerSection) {
    std::istringstream stream(headerSection);
    std::string line;
    
    // Skip request line
    std::getline(stream, line);
    
    while (std::getline(stream, line)) {
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
            
            // Trim leading spaces from value
            size_t firstNonSpace = value.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos) {
                value = value.substr(firstNonSpace);
            }
            
            headers_[toLower(name)] = value;
        }
    }
    
    // Extract Content-Length
    auto it = headers_.find("content-length");
    if (it != headers_.end()) {
        contentLength_ = std::stoull(it->second);
    }
}

std::string HttpRequest::getHeader(const std::string& name) const {
    auto it = headers_.find(toLower(name));
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

bool HttpRequest::isComplete(const std::vector<char>& rawData) const {
    // Find header end
    size_t headerEnd = 0;
    bool found = false;
    
    for (size_t i = 0; i < rawData.size() - 3; i++) {
        if (rawData[i] == '\r' && rawData[i+1] == '\n' && 
            rawData[i+2] == '\r' && rawData[i+3] == '\n') {
            headerEnd = i + 4;
            found = true;
            break;
        }
    }
    
    if (!found) {
        for (size_t i = 0; i < rawData.size() - 1; i++) {
            if (rawData[i] == '\n' && rawData[i+1] == '\n') {
                headerEnd = i + 2;
                found = true;
                break;
            }
        }
    }
    
    if (!found) {
        return false;
    }
    
    // Extract Content-Length from headers
    std::string headerSection(rawData.begin(), rawData.begin() + headerEnd);
    std::istringstream stream(headerSection);
    std::string line;
    size_t contentLength = 0;
    
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        std::string lowerLine = toLower(line);
        if (lowerLine.find("content-length:") == 0) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string value = line.substr(colonPos + 1);
                size_t firstNonSpace = value.find_first_not_of(" \t");
                if (firstNonSpace != std::string::npos) {
                    value = value.substr(firstNonSpace);
                    contentLength = std::stoull(value);
                }
            }
            break;
        }
    }
    
    // Check if we have all the data
    size_t expectedSize = headerEnd + contentLength;
    return rawData.size() >= expectedSize;
}

std::string HttpRequest::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
