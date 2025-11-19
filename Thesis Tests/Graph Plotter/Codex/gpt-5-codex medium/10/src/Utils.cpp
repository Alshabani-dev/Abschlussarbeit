#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

int hexValue(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

}

namespace utils {

std::string urlDecode(const std::string &value) {
    std::string decoded;
    decoded.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '+') {
            decoded.push_back(' ');
        } else if (c == '%' && i + 2 < value.size()) {
            int hi = hexValue(value[i + 1]);
            int lo = hexValue(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                decoded.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
            } else {
                decoded.push_back(c);
            }
        } else {
            decoded.push_back(c);
        }
    }
    return decoded;
}

std::string trim(const std::string &value) {
    if (value.empty()) {
        return value;
    }
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::vector<std::string> split(const std::string &input, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;
    std::istringstream stream(input);
    while (std::getline(stream, current, delimiter)) {
        tokens.push_back(current);
    }
    if (input.empty()) {
        tokens.clear();
    }
    return tokens;
}

bool parseDoubleList(const std::string &input, std::vector<double> &result) {
    result.clear();
    if (input.empty()) {
        return false;
    }
    std::vector<std::string> tokens = split(input, ',');
    for (const auto &token : tokens) {
        std::string trimmed = trim(token);
        if (trimmed.empty()) {
            return false;
        }
        char *endPtr = nullptr;
        errno = 0;
        double value = std::strtod(trimmed.c_str(), &endPtr);
        if (errno != 0 || endPtr == trimmed.c_str() || *endPtr != '\0') {
            return false;
        }
        result.push_back(value);
    }
    return !result.empty();
}

std::string readFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}
