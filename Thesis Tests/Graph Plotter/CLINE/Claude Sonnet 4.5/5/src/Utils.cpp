#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>
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
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    
    return (start < end) ? std::string(start, end) : std::string();
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 
           str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<double> parseDoubleArray(const std::string& str) {
    std::vector<double> result;
    std::vector<std::string> tokens = split(str, ',');
    
    for (const std::string& token : tokens) {
        std::string trimmed = trim(token);
        if (trimmed.empty()) continue;
        
        try {
            double value = std::stod(trimmed);
            result.push_back(value);
        } catch (const std::exception&) {
            // Invalid number - skip it
            continue;
        }
    }
    
    return result;
}

std::string urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            // Decode %XX hex encoding
            int value;
            std::stringstream ss;
            ss << std::hex << str.substr(i + 1, 2);
            ss >> value;
            result += static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            // Convert + to space
            result += ' ';
        } else {
            result += str[i];
        }
    }
    
    return result;
}

std::string formatNumber(double value, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

}
