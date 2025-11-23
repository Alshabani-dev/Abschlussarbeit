#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace Utils {

// Convert string to uppercase
inline std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Convert string to lowercase
inline std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Trim whitespace from both ends
inline std::string trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(str[start])) {
        ++start;
    }
    
    while (end > start && std::isspace(str[end - 1])) {
        --end;
    }
    
    return str.substr(start, end - start);
}

// Split string by delimiter
inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (char ch : str) {
        if (ch == delimiter) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += ch;
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}

// Escape string for CSV (handle commas, quotes, newlines)
inline std::string escapeCsv(const std::string& str) {
    if (str.find(',') == std::string::npos && 
        str.find('"') == std::string::npos && 
        str.find('\n') == std::string::npos) {
        return str;
    }
    
    std::string result = "\"";
    for (char ch : str) {
        if (ch == '"') {
            result += "\"\"";  // Double the quote
        } else {
            result += ch;
        }
    }
    result += "\"";
    return result;
}

// Unescape CSV field
inline std::string unescapeCsv(const std::string& str) {
    if (str.empty() || str[0] != '"') {
        return str;
    }
    
    std::string result;
    bool inQuote = false;
    
    for (size_t i = 1; i < str.length() - 1; ++i) {
        if (str[i] == '"' && i + 1 < str.length() - 1 && str[i + 1] == '"') {
            result += '"';
            ++i;  // Skip next quote
        } else {
            result += str[i];
        }
    }
    
    return result;
}

// Check if string is a number
inline bool isNumber(const std::string& str) {
    if (str.empty()) return false;
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        start = 1;
    }
    
    bool hasDigit = false;
    bool hasDecimal = false;
    
    for (size_t i = start; i < str.length(); ++i) {
        if (std::isdigit(str[i])) {
            hasDigit = true;
        } else if (str[i] == '.' && !hasDecimal) {
            hasDecimal = true;
        } else {
            return false;
        }
    }
    
    return hasDigit;
}

} // namespace Utils

#endif // UTILS_H
