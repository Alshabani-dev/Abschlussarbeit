#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Utils {

std::string trim(const std::string &value) {
    auto begin = value.begin();
    while (begin != value.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }

    auto end = value.end();
    do {
        if (end == begin) {
            break;
        }
        --end;
    } while (std::isspace(static_cast<unsigned char>(*end)));

    if (begin == value.end()) {
        return "";
    }
    return std::string(begin, end + 1);
}

std::vector<std::string> split(const std::string &value, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

static int hexToInt(char c) {
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

std::string urlDecode(const std::string &value) {
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        if (c == '+') {
            result.push_back(' ');
        } else if (c == '%' && i + 2 < value.size()) {
            int hi = hexToInt(value[i + 1]);
            int lo = hexToInt(value[i + 2]);
            if (hi >= 0 && lo >= 0) {
                result.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
            }
        } else {
            result.push_back(c);
        }
    }
    return result;
}

std::vector<double> parseNumberList(const std::string &value) {
    std::vector<double> numbers;
    std::string token;
    auto flushToken = [&](bool force) {
        if (!token.empty()) {
            try {
                numbers.push_back(std::stod(token));
            } catch (const std::exception &) {
                // Ignore invalid numbers to avoid crashing the server
            }
            token.clear();
        } else if (force) {
            token.clear();
        }
    };

    for (char c : value) {
        if (c == ',' || c == '\n' || c == '\r' || c == ';' || std::isspace(static_cast<unsigned char>(c))) {
            flushToken(false);
        } else {
            token.push_back(c);
        }
    }
    flushToken(true);
    return numbers;
}

std::string readFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string readFileFromCandidates(const std::vector<std::string> &candidates) {
    for (const auto &path : candidates) {
        const std::string data = readFile(path);
        if (!data.empty()) {
            return data;
        }
    }
    return {};
}

std::map<std::string, std::string> parseFormURLEncoded(const std::string &body) {
    std::map<std::string, std::string> params;
    const auto pairs = split(body, '&');
    for (const auto &pair : pairs) {
        auto parts = split(pair, '=');
        if (parts.empty()) {
            continue;
        }
        const std::string key = urlDecode(parts[0]);
        std::string value;
        if (parts.size() > 1) {
            value = urlDecode(parts[1]);
        }
        params[key] = value;
    }
    return params;
}

} // namespace Utils
