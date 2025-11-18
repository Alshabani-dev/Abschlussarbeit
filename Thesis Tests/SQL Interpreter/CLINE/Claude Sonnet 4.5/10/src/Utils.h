#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace Utils {
    // Trim whitespace from both ends
    inline std::string trim(const std::string& str) {
        size_t start = 0;
        size_t end = str.length();
        
        while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
            start++;
        }
        
        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
            end--;
        }
        
        return str.substr(start, end - start);
    }
    
    // Convert string to lowercase
    inline std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    }
    
    // Convert string to uppercase
    inline std::string toUpper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::toupper(c); });
        return result;
    }
    
    // Split string by delimiter
    inline std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    // Escape CSV value (wrap in quotes if contains comma, quote, or newline)
    inline std::string escapeCsv(const std::string& value) {
        bool needsQuotes = false;
        std::string result;
        
        // Check if value contains special characters
        for (char c : value) {
            if (c == ',' || c == '"' || c == '\n' || c == '\r') {
                needsQuotes = true;
            }
            
            // Double-escape quotes
            if (c == '"') {
                result += '"';
                result += '"';
            } else {
                result += c;
            }
        }
        
        if (needsQuotes) {
            return '"' + result + '"';
        }
        
        return result;
    }
    
    // Unescape CSV value (remove quotes and un-double quotes)
    inline std::string unescapeCsv(const std::string& value) {
        if (value.empty()) {
            return value;
        }
        
        std::string result = value;
        
        // Remove surrounding quotes if present
        if (result.length() >= 2 && result.front() == '"' && result.back() == '"') {
            result = result.substr(1, result.length() - 2);
            
            // Un-double quotes
            size_t pos = 0;
            while ((pos = result.find("\"\"", pos)) != std::string::npos) {
                result.replace(pos, 2, "\"");
                pos += 1;
            }
        }
        
        return result;
    }
}

#endif // UTILS_H
