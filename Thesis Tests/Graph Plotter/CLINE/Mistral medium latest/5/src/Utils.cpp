#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <stdexcept>

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
        const auto first = str.find_first_not_of(" \t\n\r");
        if (std::string::npos == first) {
            return "";
        }
        const auto last = str.find_last_not_of(" \t\n\r");
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
                result.push_back(std::stod(trim(token)));
            } catch (const std::invalid_argument&) {
                // Skip invalid tokens
            } catch (const std::out_of_range&) {
                // Skip out-of-range tokens
            }
        }
        return result;
    }

    std::string urlDecode(const std::string& str) {
        std::ostringstream decoded;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%' && i + 2 < str.size()) {
                int hex;
                std::istringstream hexStream(str.substr(i + 1, 2));
                if (hexStream >> std::hex >> hex) {
                    decoded << static_cast<char>(hex);
                    i += 2;
                } else {
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
