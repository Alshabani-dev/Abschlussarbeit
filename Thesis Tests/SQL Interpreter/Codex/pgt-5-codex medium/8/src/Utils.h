#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace utils {

inline std::string trim(const std::string &input) {
    auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}

inline std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

inline std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

inline std::string join(const std::vector<std::string> &items, const std::string &delim) {
    std::ostringstream oss;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i != 0) {
            oss << delim;
        }
        oss << items[i];
    }
    return oss.str();
}

inline std::string csvEscape(const std::string &value) {
    bool needsQuotes = value.find_first_of(",\"\n\r") != std::string::npos;
    std::string escaped;
    for (char c : value) {
        if (c == '"') {
            escaped += "\"\"";
        } else {
            escaped += c;
        }
    }
    if (needsQuotes) {
        return "\"" + escaped + "\"";
    }
    return escaped;
}

inline std::vector<std::string> parseCsvLine(const std::string &line) {
    std::vector<std::string> values;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                values.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
    }
    values.push_back(current);
    return values;
}

} // namespace utils

#endif // UTILS_H
