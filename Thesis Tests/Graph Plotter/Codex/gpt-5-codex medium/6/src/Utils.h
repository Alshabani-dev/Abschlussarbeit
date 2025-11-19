#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <unordered_map>

namespace Utils {

std::string trim(const std::string& value);
std::vector<std::string> split(const std::string& value, char delimiter);
std::string urlDecode(const std::string& value);
std::vector<double> parseDoubleList(const std::string& value);
std::unordered_map<std::string, std::string> parseFormUrlEncoded(const std::string& body);
std::string readFile(const std::string& path);

}

#endif
