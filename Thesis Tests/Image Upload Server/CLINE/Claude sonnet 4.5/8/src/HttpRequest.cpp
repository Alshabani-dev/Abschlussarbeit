#include "HttpRequest.h"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest(const std::vector<char>& rawRequest) {
    parse(rawRequest);
}

std::string HttpRequest::getMethod() const {
    return method_;
}

std::string HttpRequest::getPath() const {
    return path_;
}

std::string HttpRequest::getHeader(const std::string& name) const {
    std::string lowerName = toLower(name);
    auto it = headers_.find(lowerName);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::vector<char> HttpRequest::getBody() const {
    return body_;
}

std::string HttpRequest::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void HttpRequest::parse(const std::vector<char>& rawRequest) {
    if (rawRequest.empty()) {
        return;
    }

    // Convert to string for header parsing
    std::string requestStr(rawRequest.begin(), rawRequest.end());
    
    // Find end of headers (supports both \r\n\r\n and \n\n)
    size_t headerEnd = requestStr.find("\r\n\r\n");
    size_t headerEndSize = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = requestStr.find("\n\n");
        headerEndSize = 2;
        if (headerEnd == std::string::npos) {
            headerEnd = requestStr.size();
            headerEndSize = 0;
        }
    }
    
    // Extract headers string
    std::string headersStr = requestStr.substr(0, headerEnd);
    
    // Parse request line
    std::istringstream headerStream(headersStr);
    std::string line;
    if (std::getline(headerStream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        std::istringstream lineStream(line);
        lineStream >> method_ >> path_ >> version_;
    }
    
    // Parse headers
    while (std::getline(headerStream, line)) {
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
            
            // Trim leading whitespace from value
            size_t valueStart = value.find_first_not_of(" \t");
            if (valueStart != std::string::npos) {
                value = value.substr(valueStart);
            }
            
            headers_[toLower(name)] = value;
        }
    }
    
    // Extract body (binary-safe using vector)
    if (headerEnd + headerEndSize < rawRequest.size()) {
        body_.assign(rawRequest.begin() + headerEnd + headerEndSize, rawRequest.end());
    }
}
