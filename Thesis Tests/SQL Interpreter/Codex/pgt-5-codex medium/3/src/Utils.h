#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {

std::string trim(const std::string &value);
std::string toUpper(const std::string &value);
bool isIdentifierChar(char c);
std::vector<std::string> parseCsvLine(const std::string &line);
std::string buildCsvLine(const std::vector<std::string> &values);
bool endsWithStatementTerminator(const std::string &text);

}

#endif // UTILS_H
