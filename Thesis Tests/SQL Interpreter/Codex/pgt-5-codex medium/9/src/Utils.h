#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace Utils {

inline std::string trim(const std::string &input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

inline std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

inline std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

inline std::vector<std::string> splitStatements(const std::string &input) {
    std::vector<std::string> statements;
    std::string current;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
        } else if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
        }

        if (c == ';' && !inSingleQuote && !inDoubleQuote) {
            std::string trimmed = trim(current);
            if (!trimmed.empty()) {
                statements.push_back(trimmed);
            }
            current.clear();
        } else {
            current.push_back(c);
        }
    }

    std::string trimmed = trim(current);
    if (!trimmed.empty()) {
        statements.push_back(trimmed);
    }

    return statements;
}

} // namespace Utils

#endif // UTILS_H
