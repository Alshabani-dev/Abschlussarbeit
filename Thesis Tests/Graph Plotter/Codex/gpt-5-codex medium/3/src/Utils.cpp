#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

std::string trim(const std::string& value) {
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
    std::string decoded;
    decoded.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            std::string hex = value.substr(i + 1, 2);
            char c = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            decoded += c;
            i += 2;
        } else if (value[i] == '+') {
            decoded += ' ';
        } else {
            decoded += value[i];
        }
    }
    return decoded;
}

std::vector<double> parseNumberList(const std::string& value) {
    std::vector<double> numbers;
    for (const std::string& raw : split(value, ',')) {
        std::string token = trim(raw);
        if (token.empty()) {
            continue;
        }
        try {
            numbers.push_back(std::stod(token));
        } catch (...) {
            // Ignore invalid entries
        }
    }
    return numbers;
}

std::unordered_map<std::string, std::string> parseFormData(const std::string& body) {
    std::unordered_map<std::string, std::string> fields;
    for (const std::string& part : split(body, '&')) {
        size_t eqPos = part.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }
        std::string key = urlDecode(part.substr(0, eqPos));
        std::string value = urlDecode(part.substr(eqPos + 1));
        fields[key] = value;
    }
    return fields;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
