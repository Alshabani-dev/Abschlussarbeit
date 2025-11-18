#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
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

struct SplitResult {
    std::vector<std::string> statements;
    std::string remainder;
};

inline SplitResult splitSqlStatements(const std::string &input) {
    SplitResult result;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    size_t start = 0;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
        } else if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
        } else if (c == ';' && !inSingleQuote && !inDoubleQuote) {
            result.statements.emplace_back(input.substr(start, i - start));
            start = i + 1;
        }
    }
    if (start < input.size()) {
        result.remainder = input.substr(start);
    } else {
        result.remainder.clear();
    }
    return result;
}

inline bool hasStatementTerminator(const std::string &input) {
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    for (char c : input) {
        if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
        } else if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
        } else if (c == ';' && !inSingleQuote && !inDoubleQuote) {
            return true;
        }
    }
    return false;
}

inline std::string urlDecode(const std::string &data) {
    std::string decoded;
    decoded.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        char c = data[i];
        if (c == '+') {
            decoded.push_back(' ');
        } else if (c == '%' && i + 2 < data.size()) {
            std::string hex = data.substr(i + 1, 2);
            char *end = nullptr;
            long value = std::strtol(hex.c_str(), &end, 16);
            if (end != nullptr && *end == '\0') {
                decoded.push_back(static_cast<char>(value));
                i += 2;
            }
        } else {
            decoded.push_back(c);
        }
    }
    return decoded;
}

inline std::string htmlEscape(const std::string &input) {
    std::string output;
    output.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '&': output += "&amp;"; break;
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            case '"': output += "&quot;"; break;
            case '\'': output += "&#39;"; break;
            default: output.push_back(c); break;
        }
    }
    return output;
}

inline std::vector<std::string> joinStatements(const SplitResult &split) {
    std::vector<std::string> statements = split.statements;
    std::string leftover = trim(split.remainder);
    if (!leftover.empty()) {
        statements.push_back(leftover);
    }
    for (std::string &stmt : statements) {
        stmt = trim(stmt);
    }
    return statements;
}

} // namespace Utils

#endif // UTILS_H
