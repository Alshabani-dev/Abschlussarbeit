#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace utils {

std::string trim(const std::string& input) {
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

std::vector<std::string> split(const std::string& input, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    std::istringstream stream(input);
    while (std::getline(stream, current, delimiter)) {
        result.push_back(current);
    }
    return result;
}

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string urlDecode(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '+') {
            result.push_back(' ');
        } else if (c == '%' && i + 2 < input.size()) {
            int hi = hexValue(input[i + 1]);
            int lo = hexValue(input[i + 2]);
            if (hi >= 0 && lo >= 0) {
                result.push_back(static_cast<char>((hi << 4) | lo));
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

std::vector<double> parseNumberList(const std::string& input) {
    std::vector<double> result;
    std::string token;
    std::stringstream ss(input);
    while (std::getline(ss, token, ',')) {
        std::stringstream inner(token);
        std::string piece;
        while (inner >> piece) {
            try {
                result.push_back(std::stod(piece));
            } catch (...) {
                // skip invalid parts
            }
        }
    }
    return result;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}
