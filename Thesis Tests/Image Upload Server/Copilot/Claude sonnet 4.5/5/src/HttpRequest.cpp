#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cstring>

HttpRequest::HttpRequest() : contentLength_(0) {}

std::string HttpRequest::toLowerCase(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool HttpRequest::isComplete(const std::vector<char>& buffer) const {
    // Find header/body separator
    std::string bufferStr(buffer.begin(), buffer.end());
    size_t headerEnd = bufferStr.find("\r\n\r\n");
    size_t headerSize = 0;
    
    if (headerEnd != std::string::npos) {
        headerSize = headerEnd + 4;
    } else {
        headerEnd = bufferStr.find("\n\n");
        if (headerEnd != std::string::npos) {
            headerSize = headerEnd + 2;
        } else {
            return false; // Headers not complete yet
        }
    }
    
    // Extract Content-Length from headers
    size_t clPos = bufferStr.find("Content-Length:");
    if (clPos == std::string::npos) {
        clPos = bufferStr.find("content-length:");
    }
    
    if (clPos != std::string::npos && clPos < headerEnd) {
        size_t clEnd = bufferStr.find("\r\n", clPos);
        if (clEnd == std::string::npos) {
            clEnd = bufferStr.find("\n", clPos);
        }
        
        std::string clValue = bufferStr.substr(clPos + 15, clEnd - clPos - 15);
        // Trim whitespace
        clValue.erase(0, clValue.find_first_not_of(" \t\r\n"));
        clValue.erase(clValue.find_last_not_of(" \t\r\n") + 1);
        
        int expectedLength = std::stoi(clValue);
        size_t actualBodySize = buffer.size() - headerSize;
        
        return actualBodySize >= static_cast<size_t>(expectedLength);
    }
    
    // No Content-Length header (GET/HEAD request)
    return true;
}

bool HttpRequest::parse(const std::vector<char>& data) {
    // Find header/body separator
    std::string dataStr(data.begin(), data.end());
    size_t headerEnd = dataStr.find("\r\n\r\n");
    size_t headerSize = 0;
    
    if (headerEnd != std::string::npos) {
        headerSize = headerEnd + 4;
    } else {
        headerEnd = dataStr.find("\n\n");
        if (headerEnd != std::string::npos) {
            headerSize = headerEnd + 2;
        } else {
            return false; // Headers not complete
        }
    }
    
    // Extract headers
    std::string headerSection = dataStr.substr(0, headerEnd);
    
    // Parse request line
    size_t firstLine = headerSection.find('\n');
    std::string requestLine = headerSection.substr(0, firstLine);
    
    // Remove \r if present
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }
    
    std::istringstream iss(requestLine);
    iss >> method_ >> path_ >> version_;
    
    // Parse headers
    parseHeaders(headerSection.substr(firstLine + 1));
    
    // Extract Content-Length
    std::string clHeader = getHeader("content-length");
    if (!clHeader.empty()) {
        contentLength_ = std::stoi(clHeader);
    } else {
        contentLength_ = 0;
    }
    
    // Extract body (binary-safe using vector)
    if (data.size() > headerSize) {
        body_.assign(data.begin() + headerSize, data.end());
    }
    
    return true;
}

void HttpRequest::parseHeaders(const std::string& headerSection) {
    std::istringstream stream(headerSection);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) continue;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace from value
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            headers_[toLowerCase(name)] = value;
        }
    }
}

std::string HttpRequest::getHeader(const std::string& name) const {
    auto it = headers_.find(toLowerCase(name));
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}
