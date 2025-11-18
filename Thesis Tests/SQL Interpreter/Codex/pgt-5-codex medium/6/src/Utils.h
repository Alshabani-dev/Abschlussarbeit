#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace Utils {

inline std::string trim(const std::string &value) {
    size_t start = 0;
    size_t end = value.size();
    while (start < end && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

inline std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

inline std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

inline std::vector<std::string> split(const std::string &input, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(input);
    std::string piece;
    while (std::getline(ss, piece, delimiter)) {
        parts.push_back(piece);
    }
    return parts;
}

inline std::string join(const std::vector<std::string> &values, const std::string &delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << delimiter;
        }
        oss << values[i];
    }
    return oss.str();
}

} // namespace Utils

#endif // UTILS_H
