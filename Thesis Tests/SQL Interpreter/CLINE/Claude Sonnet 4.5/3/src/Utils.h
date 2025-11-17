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
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

// Convert to uppercase
inline std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

// Convert to lowercase
inline std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
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

// Escape CSV value (wrap in quotes if contains comma, quote, or newline)
inline std::string escapeCSV(const std::string& value) {
    bool needsQuotes = (value.find(',') != std::string::npos ||
                        value.find('"') != std::string::npos ||
                        value.find('\n') != std::string::npos);
    
    if (!needsQuotes) {
        return value;
    }
    
    std::string result = "\"";
    for (char c : value) {
        if (c == '"') {
            result += "\"\""; // Escape quotes by doubling
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

// Unescape CSV value
inline std::string unescapeCSV(const std::string& value) {
    if (value.empty()) return value;
    
    // Check if wrapped in quotes
    if (value.front() == '"' && value.back() == '"' && value.length() >= 2) {
        std::string result;
        for (size_t i = 1; i < value.length() - 1; ++i) {
            if (value[i] == '"' && i + 1 < value.length() - 1 && value[i + 1] == '"') {
                result += '"';
                ++i; // Skip next quote
            } else {
                result += value[i];
            }
        }
        return result;
    }
    
    return value;
}

// Parse CSV line respecting quoted values
inline std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> values;
    std::string current;
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                current += '"';
                ++i; // Skip next quote
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            values.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    values.push_back(current);
    
    return values;
}

} // namespace Utils

#endif // UTILS_H
