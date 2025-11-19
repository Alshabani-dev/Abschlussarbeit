#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace Utils {

std::string trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t\n\r");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        parts.push_back(item);
    }
    return parts;
}

std::string urlDecode(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            result.push_back(' ');
        } else if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char decoded = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result.push_back(decoded);
            i += 2;
        } else {
            result.push_back(value[i]);
        }
    }
    return result;
}

std::vector<double> parseDoubleList(const std::string& value) {
    std::vector<double> numbers;
    std::string token;
    auto flushToken = [&]() {
        std::string trimmed = trim(token);
        if (!trimmed.empty()) {
            try {
                numbers.push_back(std::stod(trimmed));
            } catch (...) {
                // Ignore invalid numbers
            }
        }
        token.clear();
    };

    for (char c : value) {
        if (c == ',' || c == ';' || c == '\n' || c == '\t') {
            flushToken();
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            if (!token.empty()) {
                flushToken();
            }
        } else {
            token.push_back(c);
        }
    }
    flushToken();
    return numbers;
}

std::unordered_map<std::string, std::string> parseFormUrlEncoded(const std::string& body) {
    std::unordered_map<std::string, std::string> result;
    const auto pairs = split(body, '&');
    for (const auto& pair : pairs) {
        const auto pos = pair.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const std::string key = urlDecode(pair.substr(0, pos));
        const std::string value = urlDecode(pair.substr(pos + 1));
        result[key] = value;
    }
    return result;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}
