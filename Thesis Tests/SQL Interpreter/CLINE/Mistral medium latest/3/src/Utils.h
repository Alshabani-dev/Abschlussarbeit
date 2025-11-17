#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>

namespace Utils {
    std::vector<std::string> split(const std::string &s, char delimiter);
    std::string trim(const std::string &s);
    std::string toLower(const std::string &s);
    std::string join(const std::vector<std::string> &strings, const std::string &delimiter);
    std::string escapeCsv(const std::string &value);
    std::string unescapeCsv(const std::string &value);
}

#endif // UTILS_H
