#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cctype>

HttpRequest::HttpRequest() {}

bool HttpRequest::parse(const std::vector<char>& rawData) {
    if (rawData.empty()) {
        return false;
    }
    
    // Find header end - support both \r\n\r\n and \n\n
    size_t headerEnd = 0;
    size_t headerSize = 0;
    
    // Try \r\n\r\n first
    for (size_t i = 0; i < rawData.size() - 3; ++i) {
        if (rawData[i] == '\r' && rawData[i+1] == '\n' && 
            rawData[i+2] == '\r' && rawData[i+3] == '\n') {
            headerEnd = i;
            headerSize = 4;
            break;
        }
    }
    
    // Fallback to \n\n
    if (headerSize == 0) {
        for (size_t i = 0; i < rawData.size() - 1; ++i) {
            if (rawData[i] == '\n' && rawData[i+1] == '\n') {
                headerEnd = i;
                headerSize = 2;
                break;
            }
        }
    }
    
    if (headerSize == 0) {
        return false; // No header delimiter found
    }
    
    // Extract headers as string
    std::string headerStr(rawData.begin(), rawData.begin() + headerEnd);
    
    // Extract body as vector (binary-safe)
    size_t bodyStart = headerEnd + headerSize;
    if (bodyStart < rawData.size()) {
        body_.assign(rawData.begin() + bodyStart, rawData.end());
    }
    
    // Parse headers line by line
    std::istringstream headerStream(headerStr);
    std::string line;
    bool firstLine = true;
    
    while (std::getline(headerStream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) {
            continue;
        }
        
        if (firstLine) {
            parseRequestLine(line);
            firstLine = false;
        } else {
            parseHeader(line);
        }
    }
    
    return true;
}

void HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream iss(line);
    iss >> method_ >> path_ >> version_;
}

void HttpRequest::parseHeader(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        std::string name = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // Trim leading spaces from value
        size_t valueStart = value.find_first_not_of(" \t");
        if (valueStart != std::string::npos) {
            value = value.substr(valueStart);
        }
        
        // Store with lowercase key for case-insensitive lookup
        headers_[toLowerCase(name)] = value;
    }
}

std::string HttpRequest::getHeader(const std::string& name) const {
    auto it = headers_.find(toLowerCase(name));
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::string HttpRequest::toLowerCase(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}
