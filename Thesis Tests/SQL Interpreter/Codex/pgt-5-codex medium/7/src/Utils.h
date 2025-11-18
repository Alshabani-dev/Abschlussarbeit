#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

inline std::string trim(const std::string &input) {
    auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}

inline std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

inline std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

inline std::vector<std::string> splitCommaList(const std::string &input) {
    std::vector<std::string> parts;
    std::string current;
    std::istringstream ss(input);
    while (std::getline(ss, current, ',')) {
        parts.emplace_back(trim(current));
    }
    return parts;
}

inline std::string csvEscape(const std::string &value) {
    bool needsQuotes = value.find_first_of(",\"\n\r") != std::string::npos;
    if (!needsQuotes) {
        return value;
    }
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (char c : value) {
        if (c == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(c);
    }
    escaped.push_back('"');
    return escaped;
}

inline std::string csvUnescape(const std::string &value) {
    if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
        return value;
    }
    std::string unescaped;
    unescaped.reserve(value.size());
    for (size_t i = 1; i + 1 < value.size(); ++i) {
        char c = value[i];
        if (c == '"' && i + 2 < value.size() && value[i + 1] == '"') {
            unescaped.push_back('"');
            ++i;
        } else {
            unescaped.push_back(c);
        }
    }
    return unescaped;
}

inline std::vector<std::string> splitCsvLine(const std::string &line) {
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
                current.push_back('"');
                ++i;
            } else if (c == '"') {
                inQuotes = false;
            } else {
                current.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                result.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
    }
    result.push_back(current);
    for (std::string &field : result) {
        field = csvUnescape(field);
    }
    return result;
}

inline std::string join(const std::vector<std::string> &values, const std::string &separator) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << separator;
        }
        oss << values[i];
    }
    return oss.str();
}

#endif // UTILS_H
