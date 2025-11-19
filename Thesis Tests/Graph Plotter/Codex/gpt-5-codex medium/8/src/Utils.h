#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {

std::string trim(const std::string &value);
std::vector<std::string> split(const std::string &value, char delimiter);
std::string urlDecode(const std::string &value);
std::vector<double> parseNumberList(const std::string &value);
std::string readFile(const std::string &path);

inline bool startsWith(const std::string &value, const std::string &prefix) {
    return value.rfind(prefix, 0) == 0;
}

} // namespace Utils

#endif
