#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace utils {

inline std::string trim(const std::string &input) {
    auto begin = input.begin();
    while (begin != input.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    auto end = input.end();
    while (end != begin) {
        auto prev = end;
        --prev;
        if (!std::isspace(static_cast<unsigned char>(*prev))) {
            break;
        }
        end = prev;
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

inline std::vector<std::string> split(const std::string &input, char delimiter) {
    std::vector<std::string> result;
    std::stringstream stream(input);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        result.emplace_back(item);
    }
    return result;
}

inline std::string join(const std::vector<std::string> &values, const std::string &delimiter) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << delimiter;
        }
        oss << values[i];
    }
    return oss.str();
}

inline bool iequals(const std::string &a, const std::string &b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace utils

#endif // UTILS_H
