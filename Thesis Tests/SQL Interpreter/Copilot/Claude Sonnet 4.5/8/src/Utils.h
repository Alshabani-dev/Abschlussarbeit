#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

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
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// Split string by delimiter
inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Escape CSV field (wrap in quotes if contains comma, quote, or newline)
inline std::string escapeCsv(const std::string& field) {
    bool needsQuotes = false;
    std::string result;
    
    for (char c : field) {
        if (c == ',' || c == '\n' || c == '\r' || c == '"') {
            needsQuotes = true;
        }
        if (c == '"') {
            result += "\"\""; // Double the quote
        } else {
            result += c;
        }
    }
    
    if (needsQuotes) {
        return "\"" + result + "\"";
    }
    return result;
}

// Unescape CSV field (remove quotes and unescape internal quotes)
inline std::string unescapeCsv(const std::string& field) {
    if (field.empty()) return field;
    
    std::string result = field;
    
    // Remove surrounding quotes if present
    if (result.size() >= 2 && result.front() == '"' && result.back() == '"') {
        result = result.substr(1, result.size() - 2);
        
        // Unescape internal quotes
        size_t pos = 0;
        while ((pos = result.find("\"\"", pos)) != std::string::npos) {
            result.replace(pos, 2, "\"");
            pos += 1;
        }
    }
    
    return result;
}

} // namespace Utils

#endif // UTILS_H
