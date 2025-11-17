#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> Utils::split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string Utils::trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

std::string Utils::toLower(const std::string& s) {
    std::string result;
    for (char c : s) {
        result += tolower(c);
    }
    return result;
}

std::string Utils::escapeCsv(const std::string& s) {
    // Only escape if the string contains commas, quotes, or newlines
    if (s.find(',') != std::string::npos || s.find('"') != std::string::npos || s.find('\n') != std::string::npos) {
        std::string result;
        for (char c : s) {
            if (c == '"') {
                result += "\"\"";
            } else {
                result += c;
            }
        }
        return "\"" + result + "\"";
    }
    return s;
}

std::string Utils::unescapeCsv(const std::string& s) {
    // If the string is not quoted, return as is
    if (s.empty() || s[0] != '"' || s.back() != '"') {
        return s;
    }

    std::string result;
    // Skip the first and last quote
    for (size_t i = 1; i < s.size() - 1; i++) {
        if (s[i] == '"' && i + 1 < s.size() - 1 && s[i+1] == '"') {
            result += '"';
            i++;
        } else {
            result += s[i];
        }
    }
    return result;
}
