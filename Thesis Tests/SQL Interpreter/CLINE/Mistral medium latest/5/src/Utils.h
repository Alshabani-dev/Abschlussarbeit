#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>

class Utils {
public:
    static std::vector<std::string> split(const std::string &s, char delimiter);
    static std::string trim(const std::string &s);
    static std::string toLower(const std::string &s);
    static std::string join(const std::vector<std::string> &strings, const std::string &delimiter);
    static std::string escapeCsvField(const std::string &field);
    static std::string unescapeCsvField(const std::string &field);
};

#endif // UTILS_H
