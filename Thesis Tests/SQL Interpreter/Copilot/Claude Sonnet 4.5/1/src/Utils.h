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

// Trim whitespace from both ends of string
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

// Escape string for CSV (wrap in quotes if needed, double internal quotes)
inline std::string escapeCsv(const std::string& str) {
    bool needsQuotes = (str.find(',') != std::string::npos ||
                       str.find('"') != std::string::npos ||
                       str.find('\n') != std::string::npos);
    
    if (!needsQuotes) {
        return str;
    }
    
    std::string result = "\"";
    for (char c : str) {
        if (c == '"') {
            result += "\"\""; // Double the quote
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

// Unescape CSV string (remove quotes, un-double internal quotes)
inline std::string unescapeCsv(const std::string& str) {
    if (str.empty()) return str;
    
    // If string is quoted, remove quotes and un-double internal quotes
    if (str.front() == '"' && str.back() == '"' && str.length() >= 2) {
        std::string result;
        bool lastWasQuote = false;
        for (size_t i = 1; i < str.length() - 1; ++i) {
            if (str[i] == '"') {
                if (lastWasQuote) {
                    result += '"';
                    lastWasQuote = false;
                } else {
                    lastWasQuote = true;
                }
            } else {
                if (lastWasQuote) {
                    result += '"';
                    lastWasQuote = false;
                }
                result += str[i];
            }
        }
        return result;
    }
    
    return str;
}

// Parse CSV line into fields (handles quoted fields with commas)
inline std::vector<std::string> parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;
    bool lastWasQuote = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                // Doubled quote inside quoted field
                field += '"';
                ++i;
            } else {
                // Toggle quote mode
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            // End of field
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    
    // Add last field
    fields.push_back(field);
    
    return fields;
}

} // namespace Utils

#endif // UTILS_H
