#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

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

// Escape string for CSV (wrap in quotes if contains comma, quote, or newline)
inline std::string escapeCsv(const std::string& value) {
    if (value.find(',') != std::string::npos ||
        value.find('"') != std::string::npos ||
        value.find('\n') != std::string::npos) {
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
    return value;
}

// Unescape CSV value (remove quotes and handle doubled quotes)
inline std::string unescapeCsv(const std::string& value) {
    if (value.empty()) return value;
    
    if (value.front() == '"' && value.back() == '"' && value.length() >= 2) {
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
    return value;
}

} // namespace Utils

#endif // UTILS_H
