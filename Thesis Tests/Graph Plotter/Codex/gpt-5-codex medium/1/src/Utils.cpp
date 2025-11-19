#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace utils {

std::string trim(const std::string& input) {
    auto begin = input.begin();
    while (begin != input.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }

    if (begin == input.end()) {
        return {};
    }

    auto end = input.end();
    do {
        --end;
    } while (std::isspace(static_cast<unsigned char>(*end)) && end != begin);

    return std::string(begin, end + 1);
}

std::vector<std::string> split(const std::string& input, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        parts.push_back(token);
    }
    return parts;
}

std::string urlDecode(const std::string& input) {
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '+') {
            result.push_back(' ');
        } else if (input[i] == '%' && i + 2 < input.size()) {
            std::string hex = input.substr(i + 1, 2);
            char decoded = static_cast<char>(std::stoi(hex, nullptr, 16));
            result.push_back(decoded);
            i += 2;
        } else {
            result.push_back(input[i]);
        }
    }

    return result;
}

double toDouble(const std::string& value, double fallback) {
    try {
        return std::stod(value);
    } catch (...) {
        return fallback;
    }
}

std::vector<double> parseDoubles(const std::string& input) {
    std::vector<double> values;
    for (const auto& token : split(input, ',')) {
        std::string trimmed = trim(token);
        if (!trimmed.empty()) {
            values.push_back(toDouble(trimmed, 0.0));
        }
    }
    return values;
}

} // namespace utils
