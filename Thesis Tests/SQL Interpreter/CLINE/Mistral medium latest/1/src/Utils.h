#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {
    std::vector<std::string> split(const std::string& s, char delimiter);
    std::string trim(const std::string& s);
    std::string toLower(const std::string& s);
    std::string escapeCsv(const std::string& s);
    std::string unescapeCsv(const std::string& s);
}

#endif // UTILS_H
