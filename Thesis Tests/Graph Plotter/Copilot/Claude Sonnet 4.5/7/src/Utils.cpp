#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <iomanip>

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
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
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
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Invalid number format: " + trimmed);
        } catch (const std::out_of_range&) {
            throw std::runtime_error("Number out of range: " + trimmed);
        }
    }
    
    return result;
}

std::string urlDecode(const std::string& str) {
    std::string result;
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.length()) {
            // Convert hex to char
            std::string hex = str.substr(i + 1, 2);
            char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += ch;
            i += 2;
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
