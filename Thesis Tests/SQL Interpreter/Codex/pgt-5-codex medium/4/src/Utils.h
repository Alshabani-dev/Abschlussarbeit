#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Utils {

inline std::string trim(const std::string &input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

inline std::string toLower(const std::string &input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

inline std::string toUpper(const std::string &input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return result;
}

inline std::vector<std::string> split(const std::string &input, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(input);
    std::string part;
    while (std::getline(ss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}

inline std::string csvEscape(const std::string &value) {
    bool needQuotes = false;
    std::string escaped;
    for (char c : value) {
        if (c == '"') {
            escaped += "\"\"";
            needQuotes = true;
        } else {
            if (c == ',' || c == '\n' || c == '\r') {
                needQuotes = true;
            }
            escaped += c;
        }
    }
    if (needQuotes || value.empty()) {
        return '"' + escaped + '"';
    }
    return escaped;
}

inline std::string csvUnescape(const std::string &value) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        std::string inner = value.substr(1, value.size() - 2);
        std::string result;
        for (size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '"' && i + 1 < inner.size() && inner[i + 1] == '"') {
                result += '"';
                ++i;
            } else {
                result += inner[i];
            }
        }
        return result;
    }
    return value;
}

inline std::vector<std::string> parseCsvLine(const std::string &line) {
    std::vector<std::string> values;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
                current += '"';
                ++i;
            } else if (c == '"') {
                inQuotes = false;
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
    for (auto &val : values) {
        val = csvUnescape(val);
    }
    return values;
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

} // namespace Utils

#endif // UTILS_H
