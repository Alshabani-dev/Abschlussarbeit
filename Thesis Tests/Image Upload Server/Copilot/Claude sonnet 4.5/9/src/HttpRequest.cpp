#include "HttpRequest.h"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest(const std::vector<char>& requestData) {
    parse(requestData);
}

void HttpRequest::parse(const std::vector<char>& requestData) {
    if (requestData.empty()) {
        return;
    }
    
    // Convert to string for header parsing
    std::string requestStr(requestData.begin(), requestData.end());
    
    // Find headers end
    size_t headersEnd = requestStr.find("\r\n\r\n");
    size_t headersEndLen = 4;
    if (headersEnd == std::string::npos) {
        headersEnd = requestStr.find("\n\n");
        headersEndLen = 2;
        if (headersEnd == std::string::npos) {
            return;
        }
    }
    
    std::string headers = requestStr.substr(0, headersEnd);
    
    // Parse request line
    size_t firstLineEnd = headers.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        firstLineEnd = headers.find("\n");
    }
    
    if (firstLineEnd != std::string::npos) {
        std::string requestLine = headers.substr(0, firstLineEnd);
        std::istringstream iss(requestLine);
        std::string version;
        iss >> method_ >> path_ >> version;
    }
    
    // Parse headers
    size_t pos = firstLineEnd + 1;
    if (headers[firstLineEnd] == '\r') pos++;
    
    while (pos < headers.length()) {
        size_t lineEnd = headers.find("\r\n", pos);
        if (lineEnd == std::string::npos) {
            lineEnd = headers.find("\n", pos);
        }
        if (lineEnd == std::string::npos) {
            break;
        }
        
        std::string line = headers.substr(pos, lineEnd - pos);
        size_t colonPos = line.find(':');
        
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            headers_[toLowerCase(key)] = value;
        }
        
        pos = lineEnd + 1;
        if (pos < headers.length() && headers[pos - 1] == '\r') pos++;
    }
    
    // Extract body (binary-safe)
    size_t bodyStart = headersEnd + headersEndLen;
    if (bodyStart < requestData.size()) {
        body_.assign(requestData.begin() + bodyStart, requestData.end());
    }
}

std::string HttpRequest::getHeader(const std::string& key) const {
    std::string lowerKey = toLowerCase(key);
    auto it = headers_.find(lowerKey);
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
