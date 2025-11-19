#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace Utils {

std::string trim(const std::string &value) {
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

std::vector<std::string> split(const std::string &value, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    std::istringstream stream(value);
    while (std::getline(stream, current, delimiter)) {
        result.push_back(current);
    }
    if (!value.empty() && value.back() == delimiter) {
        result.emplace_back();
    }
    return result;
}

static int fromHex(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    return 0;
}

std::string urlDecode(const std::string &value) {
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '%' && i + 2 < value.size()) {
            int high = fromHex(value[i + 1]);
            int low = fromHex(value[i + 2]);
            result.push_back(static_cast<char>((high << 4) | low));
            i += 2;
        } else if (c == '+') {
            result.push_back(' ');
        } else {
            result.push_back(c);
        }
    }

    return result;
}

std::vector<double> parseNumberList(const std::string &value) {
    std::vector<double> numbers;
    std::string token;
    auto flush = [&]() {
        if (token.empty()) {
            return;
        }
        try {
            double number = std::stod(token);
            numbers.push_back(number);
        } catch (...) {
            // Ignore invalid tokens
        }
        token.clear();
    };

    for (char c : value) {
        if (c == ',' || c == ';' || std::isspace(static_cast<unsigned char>(c))) {
            flush();
        } else {
            token.push_back(c);
        }
    }
    flush();
    return numbers;
}

std::string readFile(const std::string &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

} // namespace Utils
