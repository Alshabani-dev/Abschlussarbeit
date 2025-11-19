#ifndef UTILS_H
#define UTILS_H

#include <map>
#include <string>
#include <vector>

namespace utils {

std::string trim(const std::string& value);
std::vector<std::string> split(const std::string& value, char delimiter);
std::string urlDecode(const std::string& value);
bool parseNumberList(const std::string& text, std::vector<double>& outValues);
std::map<std::string, std::string> parseFormUrlEncoded(const std::string& body);
std::string readFile(const std::string& path);

} // namespace utils

#endif
