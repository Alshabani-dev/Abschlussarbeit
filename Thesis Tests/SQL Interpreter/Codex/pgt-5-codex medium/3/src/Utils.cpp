#include "Utils.h"

#include <algorithm>
#include <cctype>

namespace Utils {

std::string trim(const std::string &value) {
    auto begin = value.begin();
    while (begin != value.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }

    auto end = value.end();
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }

    if (begin == value.end()) {
        return std::string();
    }
    return std::string(begin, end);
}

std::string toUpper(const std::string &value) {
    std::string result(value.size(), '\0');
    std::transform(value.begin(), value.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return result;
}

bool isIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::vector<std::string> parseCsvLine(const std::string &line) {
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                result.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
    }
    result.push_back(current);
    return result;
}

std::string buildCsvLine(const std::vector<std::string> &values) {
    std::string result;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            result.push_back(',');
        }
        std::string field = values[i];
        std::string escaped;
        escaped.reserve(field.size() + 2);
        escaped.push_back('"');
        for (char c : field) {
            if (c == '"') {
                escaped.push_back('"');
            }
            escaped.push_back(c);
        }
        escaped.push_back('"');
        result += escaped;
    }
    return result;
}

bool endsWithStatementTerminator(const std::string &text) {
    bool inString = false;
    char stringDelimiter = '\0';
    for (char c : text) {
        if (inString) {
            if (c == stringDelimiter) {
                inString = false;
            }
            continue;
        }
        if (c == '\'' || c == '"') {
            inString = true;
            stringDelimiter = c;
        }
    }

    for (auto it = text.rbegin(); it != text.rend(); ++it) {
        char c = *it;
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        return c == ';';
    }
    return false;
}

} // namespace Utils
