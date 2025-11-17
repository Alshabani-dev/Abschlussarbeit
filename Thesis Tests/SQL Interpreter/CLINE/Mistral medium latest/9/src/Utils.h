#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

class Utils {
public:
    static std::vector<std::string> split(const std::string &s, char delimiter);
    static std::string trim(const std::string &s);
    static std::string toLower(const std::string &s);
    static std::string escapeCsv(const std::string &s);
    static std::string unescapeCsv(const std::string &s);
};

#endif // UTILS_H
