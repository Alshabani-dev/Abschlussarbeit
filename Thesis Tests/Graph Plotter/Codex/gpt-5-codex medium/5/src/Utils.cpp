#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace utils {

std::string trim(const std::string& value) {
    auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : value) {
        if (c == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(current);
    return parts;
}

std::string urlDecode(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '+') {
            result.push_back(' ');
        } else if (c == '%' && i + 2 < value.size()) {
            const char hex[] = {value[i + 1], value[i + 2], '\0'};
            char* endPtr = nullptr;
            long decoded = std::strtol(hex, &endPtr, 16);
            if (endPtr != hex) {
                result.push_back(static_cast<char>(decoded));
                i += 2;
            } else {
                result.push_back(c);
            }
        } else {
            result.push_back(c);
        }
    }
    return result;
}

bool parseNumberList(const std::string& text, std::vector<double>& outValues) {
    outValues.clear();
    if (text.empty()) {
        return false;
    }

    std::vector<std::string> tokens = split(text, ',');
    for (const std::string& token : tokens) {
        std::string cleaned = trim(token);
        if (cleaned.empty()) {
            continue;
        }
        try {
            double value = std::stod(cleaned);
            outValues.push_back(value);
        } catch (const std::exception&) {
            return false;
        }
    }
    return !outValues.empty();
}

std::map<std::string, std::string> parseFormUrlEncoded(const std::string& body) {
    std::map<std::string, std::string> fields;
    for (const std::string& pair : split(body, '&')) {
        if (pair.empty()) {
            continue;
        }
        auto separator = pair.find('=');
        std::string rawKey = pair.substr(0, separator);
        std::string rawValue = (separator == std::string::npos) ? std::string() : pair.substr(separator + 1);
        std::string key = urlDecode(rawKey);
        std::string value = urlDecode(rawValue);
        fields[key] = value;
    }
    return fields;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace utils
