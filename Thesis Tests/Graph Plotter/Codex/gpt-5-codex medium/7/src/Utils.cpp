#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace utils {

namespace {

bool isWhitespace(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

char hexToChar(char high, char low) {
    auto hexValue = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (ch >= 'a' && ch <= 'f') {
            return ch - 'a' + 10;
        }
        return -1;
    };

    int hi = hexValue(high);
    int lo = hexValue(low);
    if (hi < 0 || lo < 0) {
        return 0;
    }
    return static_cast<char>((hi << 4) | lo);
}

} // namespace

std::string trim(const std::string &value) {
    if (value.empty()) {
        return value;
    }

    size_t start = 0;
    while (start < value.size() && isWhitespace(value[start])) {
        ++start;
    }

    if (start == value.size()) {
        return "";
    }

    size_t end = value.size() - 1;
    while (end > start && isWhitespace(value[end])) {
        --end;
    }

    return value.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string &value, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;
    std::istringstream stream(value);

    while (std::getline(stream, current, delimiter)) {
        tokens.push_back(current);
    }

    return tokens;
}

std::string urlDecode(const std::string &value) {
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];

        if (c == '+') {
            result.push_back(' ');
        } else if (c == '%' && i + 2 < value.size()) {
            result.push_back(hexToChar(value[i + 1], value[i + 2]));
            i += 2;
        } else {
            result.push_back(c);
        }
    }

    return result;
}

std::vector<double> parseDoubles(const std::string &value) {
    std::vector<double> numbers;
    for (const std::string &token : split(value, ',')) {
        std::string trimmed = trim(token);
        if (trimmed.empty()) {
            continue;
        }
        try {
            numbers.push_back(std::stod(trimmed));
        } catch (const std::exception &) {
            // Skip invalid entries silently to keep parsing resilient.
        }
    }
    return numbers;
}

std::string readFile(const std::string &path, bool *ok) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        if (ok) {
            *ok = false;
        }
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (ok) {
        *ok = true;
    }
    return buffer.str();
}

std::string getContentType(const std::string &path) {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") {
        return "text/html; charset=UTF-8";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") {
        return "text/css; charset=UTF-8";
    }
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") {
        return "application/javascript";
    }
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".bmp") {
        return "image/bmp";
    }
    return "text/plain; charset=UTF-8";
}

} // namespace utils
