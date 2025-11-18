#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace utils {

inline std::string trim(const std::string &value) {
    auto begin = value.begin();
    while (begin != value.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    auto end = value.end();
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    return std::string(begin, end);
}

inline std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

inline std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

inline std::vector<std::string> split(const std::string &value, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        parts.push_back(item);
    }
    return parts;
}

inline bool iequals(const std::string &a, const std::string &b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

inline bool containsSemicolon(const std::string &value) {
    return value.find(';') != std::string::npos;
}

inline std::vector<std::string> splitStatements(const std::string &input) {
    std::vector<std::string> statements;
    std::string current;
    for (char ch : input) {
        current.push_back(ch);
        if (ch == ';') {
            statements.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        statements.push_back(current);
    }
    return statements;
}

inline std::string join(const std::vector<std::string> &items, const std::string &sep) {
    std::ostringstream oss;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            oss << sep;
        }
        oss << items[i];
    }
    return oss.str();
}

inline std::string sanitizeIdentifier(const std::string &value) {
    // Normalize identifiers to lower-case for consistent storage lookup.
    return toLower(trim(value));
}

} // namespace utils

#endif // UTILS_H
