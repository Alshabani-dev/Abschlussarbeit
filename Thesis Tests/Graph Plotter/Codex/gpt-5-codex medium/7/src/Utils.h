#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace utils {

std::string trim(const std::string &value);
std::vector<std::string> split(const std::string &value, char delimiter);
std::string urlDecode(const std::string &value);
std::vector<double> parseDoubles(const std::string &value);
std::string readFile(const std::string &path, bool *ok = nullptr);
std::string getContentType(const std::string &path);

} // namespace utils

#endif
