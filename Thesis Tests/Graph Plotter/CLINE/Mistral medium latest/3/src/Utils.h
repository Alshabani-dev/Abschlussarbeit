#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {
    // String manipulation
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string trim(const std::string& str);
    bool startsWith(const std::string& str, const std::string& prefix);
    bool endsWith(const std::string& str, const std::string& suffix);

    // Data parsing
    std::vector<double> parseDoubleArray(const std::string& str);

    // URL decoding
    std::string urlDecode(const std::string& str);

    // Number formatting
    std::string formatNumber(double value, int precision = 1);
}

#endif
