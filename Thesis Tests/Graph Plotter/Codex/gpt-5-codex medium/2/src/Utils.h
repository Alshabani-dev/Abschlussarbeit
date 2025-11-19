#ifndef UTILS_H
#define UTILS_H

#include <map>
#include <string>
#include <vector>

namespace Utils {

std::string trim(const std::string &value);
std::vector<std::string> split(const std::string &value, char delimiter);
std::string urlDecode(const std::string &value);
std::vector<double> parseNumberList(const std::string &value);
std::string readFile(const std::string &path);
std::string readFileFromCandidates(const std::vector<std::string> &candidates);
std::map<std::string, std::string> parseFormURLEncoded(const std::string &body);

}

#endif
