#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace Utils {

std::string trim(const std::string &value) {
    auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::vector<std::string> split(const std::string &value, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;
    std::stringstream ss(value);
    while (std::getline(ss, current, delimiter)) {
        tokens.push_back(current);
    }
    if (!value.empty() && value.back() == delimiter) {
        tokens.emplace_back();
    }
    return tokens;
}

std::string urlDecode(const std::string &value) {
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            std::string hex = value.substr(i + 1, 2);
            unsigned int decoded = 0;
            std::stringstream ss;
            ss << std::hex << hex;
            ss >> decoded;
            result.push_back(static_cast<char>(decoded));
            i += 2;
        } else if (value[i] == '+') {
            result.push_back(' ');
        } else {
            result.push_back(value[i]);
        }
    }

    return result;
}

static bool isDelimiter(char c) {
    switch (c) {
    case ',':
    case ';':
    case '\n':
    case '\r':
    case '\t':
    case ' ':
        return true;
    default:
        return false;
    }
}

std::vector<double> parseNumberList(const std::string &value) {
    std::vector<double> numbers;
    std::string token;

    for (char ch : value) {
        if (isDelimiter(ch)) {
            if (!token.empty()) {
                try {
                    numbers.push_back(std::stod(token));
                } catch (const std::exception &) {
                    throw std::runtime_error("Invalid number: " + token);
                }
                token.clear();
            }
        } else {
            token.push_back(ch);
        }
    }

    if (!token.empty()) {
        try {
            numbers.push_back(std::stod(token));
        } catch (const std::exception &) {
            throw std::runtime_error("Invalid number: " + token);
        }
    }

    return numbers;
}

bool parseUrlEncodedBody(const std::string &body,
                         std::unordered_map<std::string, std::string> &out) {
    out.clear();
    auto pairs = split(body, '&');
    for (const auto &pair : pairs) {
        if (pair.empty()) {
            continue;
        }
        auto kv = split(pair, '=');
        if (kv.empty()) {
            continue;
        }
        std::string key = urlDecode(kv[0]);
        std::string valuePart = kv.size() > 1 ? kv[1] : std::string();
        out[key] = urlDecode(valuePart);
    }
    return !out.empty();
}

} // namespace Utils
