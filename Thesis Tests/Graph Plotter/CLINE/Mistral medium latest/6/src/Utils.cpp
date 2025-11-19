#include "Utils.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <iomanip>

namespace Utils {

    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string trim(const std::string& str) {
        const auto first = str.find_first_not_of(" \t\n\r\f\v");
        if (std::string::npos == first) {
            return str;
        }
        const auto last = str.find_last_not_of(" \t\n\r\f\v");
        return str.substr(first, (last - first + 1));
    }

    bool startsWith(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
    }

    bool endsWith(const std::string& str, const std::string& suffix) {
        return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    std::vector<double> parseDoubleArray(const std::string& str) {
        std::vector<double> result;
        std::vector<std::string> tokens = split(str, ',');
        for (const auto& token : tokens) {
            try {
                result.push_back(std::stod(token));
            } catch (const std::invalid_argument&) {
                throw std::invalid_argument("Invalid number: " + token);
            } catch (const std::out_of_range&) {
                throw std::out_of_range("Number out of range: " + token);
            }
        }
        return result;
    }

    std::string urlDecode(const std::string& str) {
        std::ostringstream decoded;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%') {
                if (i + 2 >= str.size()) {
                    decoded << str[i];
                    continue;
                }
                std::string hex = str.substr(i + 1, 2);
                try {
                    int value = std::stoi(hex, nullptr, 16);
                    decoded << static_cast<char>(value);
                    i += 2;
                } catch (...) {
                    decoded << str[i];
                }
            } else if (str[i] == '+') {
                decoded << ' ';
            } else {
                decoded << str[i];
            }
        }
        return decoded.str();
    }

    std::string formatNumber(double value, int precision) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << value;
        return oss.str();
    }
}
