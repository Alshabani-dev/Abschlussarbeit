#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <unordered_map>
#include <vector>

namespace utils {

std::string trim(const std::string& input);
std::vector<std::string> split(const std::string& input, char delimiter);
std::string urlDecode(const std::string& input);
double toDouble(const std::string& value, double fallback = 0.0);
std::vector<double> parseDoubles(const std::string& input);

}

#endif
