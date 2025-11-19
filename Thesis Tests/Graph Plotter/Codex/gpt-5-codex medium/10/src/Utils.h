#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace utils {

std::string urlDecode(const std::string &value);
std::string trim(const std::string &value);
std::vector<std::string> split(const std::string &input, char delimiter);
bool parseDoubleList(const std::string &input, std::vector<double> &result);
std::string readFile(const std::string &path);

}

#endif
