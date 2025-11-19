#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace utils {

std::string trim(const std::string& input);
std::vector<std::string> split(const std::string& input, char delimiter);
std::string urlDecode(const std::string& input);
std::vector<double> parseNumberList(const std::string& input);
std::string readFile(const std::string& path);

}

#endif
