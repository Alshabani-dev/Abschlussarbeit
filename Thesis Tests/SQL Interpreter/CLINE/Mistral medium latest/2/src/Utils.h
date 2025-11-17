#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {
    std::vector<std::string> split(const std::string &s, char delimiter);
    std::string trim(const std::string &s);
    std::string toLower(const std::string &s);
    std::string escapeCsvField(const std::string &field);
    std::string unescapeCsvField(const std::string &field);
}

#endif // UTILS_H
