#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> Utils::split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string Utils::trim(const std::string &s) {
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

std::string Utils::toLower(const std::string &s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string Utils::escapeCsv(const std::string &s) {
    std::ostringstream escaped;
    for (char c : s) {
        if (c == '"') {
            escaped << "\"\"";
        } else if (c == ',') {
            escaped << "\"";
        } else {
            escaped << c;
        }
    }
    return escaped.str();
}

std::string Utils::unescapeCsv(const std::string &s) {
    std::ostringstream unescaped;
    bool inQuotes = false;

    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '"') {
            if (inQuotes && i + 1 < s.size() && s[i + 1] == '"') {
                // Escaped quote
                unescaped << '"';
                i++; // Skip next quote
            } else {
                inQuotes = !inQuotes;
            }
        } else {
            unescaped << s[i];
        }
    }

    return unescaped.str();
}
