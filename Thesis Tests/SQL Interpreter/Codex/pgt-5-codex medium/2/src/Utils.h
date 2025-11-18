#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace Utils {

inline std::string trim(const std::string &input) {
    std::size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    std::size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

inline std::string toUpper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return text;
}

inline std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

inline std::vector<std::string> split(const std::string &input, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        parts.push_back(item);
    }
    return parts;
}

inline std::string join(const std::vector<std::string> &items, const std::string &separator) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            oss << separator;
        }
        oss << items[i];
    }
    return oss.str();
}

inline std::string csvEscape(const std::string &value) {
    bool needsQuotes = false;
    for (char ch : value) {
        if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r') {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes) {
        return value;
    }
    std::string escaped = "\"";
    for (char ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
        } else {
            escaped += ch;
        }
    }
    escaped += "\"";
    return escaped;
}

inline std::vector<std::string> parseCsvLine(const std::string &line) {
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (inQuotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current.push_back(ch);
            }
        } else {
            if (ch == '"') {
                inQuotes = true;
            } else if (ch == ',') {
                result.push_back(current);
                current.clear();
            } else {
                current.push_back(ch);
            }
        }
    }
    result.push_back(current);
    return result;
}

} // namespace Utils

#endif // UTILS_H
