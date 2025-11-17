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
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

// Convert string to lowercase
inline std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Trim whitespace from both ends
inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
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

// Escape CSV value (wrap in quotes if needed, double internal quotes)
inline std::string escapeCSV(const std::string& value) {
    bool needsEscape = (value.find(',') != std::string::npos ||
                        value.find('"') != std::string::npos ||
                        value.find('\n') != std::string::npos);
    
    if (!needsEscape) {
        return value;
    }
    
    std::string escaped = "\"";
    for (char c : value) {
        if (c == '"') {
            escaped += "\"\"";  // Double the quote
        } else {
            escaped += c;
        }
    }
    escaped += "\"";
    return escaped;
}

// Unescape CSV value (remove quotes, un-double internal quotes)
inline std::string unescapeCSV(const std::string& value) {
    if (value.empty() || value[0] != '"') {
        return value;
    }
    
    std::string unescaped;
    for (size_t i = 1; i < value.length() - 1; ++i) {
        if (value[i] == '"' && i + 1 < value.length() - 1 && value[i + 1] == '"') {
            unescaped += '"';
            ++i;  // Skip next quote
        } else {
            unescaped += value[i];
        }
    }
    return unescaped;
}

// Remove quotes from string literal
inline std::string removeQuotes(const std::string& str) {
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

} // namespace Utils

#endif // UTILS_H
