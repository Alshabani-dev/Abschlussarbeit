#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <stdexcept>

namespace Utils {

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    // Find first non-whitespace
    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    
    // Find last non-whitespace
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    
    return str.substr(start, end - start);
}

bool startsWith(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) {
        return false;
    }
    return str.compare(0, prefix.length(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::vector<double> parseDoubleArray(const std::string& str) {
    std::vector<double> result;
    std::vector<std::string> tokens = split(str, ',');
    
    for (const auto& token : tokens) {
        std::string trimmed = trim(token);
        if (trimmed.empty()) {
            continue;
        }
        
        try {
            double value = std::stod(trimmed);
            result.push_back(value);
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid numeric value: " + trimmed);
        }
    }
    
    return result;
}

std::string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            // Hex decode
            std::string hex = str.substr(i + 1, 2);
            try {
                int value = std::stoi(hex, nullptr, 16);
                result += static_cast<char>(value);
                i += 2;
            } catch (...) {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    
    return result;
}

std::string formatNumber(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

} // namespace Utils
