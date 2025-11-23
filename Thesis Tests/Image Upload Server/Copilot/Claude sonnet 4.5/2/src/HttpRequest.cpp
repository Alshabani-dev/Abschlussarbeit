#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cstring>

HttpRequest::HttpRequest() : is_complete_(false) {}

bool HttpRequest::parse(const std::vector<char>& raw_data) {
    if (raw_data.empty()) {
        return false;
    }
    
    // Convert to string for header parsing (safe since headers are text)
    std::string data(raw_data.begin(), raw_data.end());
    
    // Find header/body separator (try \r\n\r\n first, then \n\n)
    size_t header_end = data.find("\r\n\r\n");
    size_t header_length = 0;
    
    if (header_end != std::string::npos) {
        header_length = header_end + 4;
    } else {
        header_end = data.find("\n\n");
        if (header_end != std::string::npos) {
            header_length = header_end + 2;
        } else {
            // Headers not complete yet
            return false;
        }
    }
    
    // Parse headers
    std::string header_section = data.substr(0, header_end);
    std::istringstream stream(header_section);
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
    parseHeaders(header_section);
    
    // Extract body (binary-safe)
    if (header_length < raw_data.size()) {
        body_.assign(raw_data.begin() + header_length, raw_data.end());
    }
    
    // Check if request is complete based on Content-Length
    size_t content_length = getContentLength();
    
    // For GET/HEAD requests, no body expected
    if (method_ == "GET" || method_ == "HEAD") {
        is_complete_ = true;
        return true;
    }
    
    // For POST/PUT, check if we have the full body
    if (content_length > 0) {
        is_complete_ = (body_.size() >= content_length);
    } else {
        is_complete_ = true;
    }
    
    return is_complete_;
}

void HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream stream(line);
    stream >> method_ >> path_ >> version_;
}

void HttpRequest::parseHeaders(const std::string& header_section) {
    std::istringstream stream(header_section);
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
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim leading/trailing whitespace from value
            size_t first = value.find_first_not_of(" \t");
            size_t last = value.find_last_not_of(" \t");
            if (first != std::string::npos) {
                value = value.substr(first, (last - first + 1));
            }
            
            headers_[toLower(key)] = value;
        }
    }
}

std::string HttpRequest::getHeader(const std::string& key) const {
    std::string lower_key = toLower(key);
    auto it = headers_.find(lower_key);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

size_t HttpRequest::getContentLength() const {
    std::string content_length = getHeader("Content-Length");
    if (content_length.empty()) {
        return 0;
    }
    try {
        return std::stoul(content_length);
    } catch (...) {
        return 0;
    }
}

std::string HttpRequest::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
