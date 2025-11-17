#include "Utils.h"
#include <sstream>
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

std::string Utils::join(const std::vector<std::string> &strings, const std::string &delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i != 0) {
            oss << delimiter;
        }
        oss << strings[i];
    }
    return oss.str();
}

std::string Utils::escapeCsv(const std::string &value) {
    std::ostringstream oss;
    for (char c : value) {
        if (c == '"') {
            oss << "\"\"";
        } else if (c == ',') {
            oss << "\"";
        } else if (c == '\n') {
            oss << "\"";
        } else {
            oss << c;
        }
    }
    return oss.str();
}

std::string Utils::unescapeCsv(const std::string &value) {
    std::ostringstream oss;
    bool inQuotes = false;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '"') {
            if (i + 1 < value.size() && value[i+1] == '"') {
                oss << '"';
                i++; // Skip the next quote
            } else {
                inQuotes = !inQuotes;
            }
        } else {
            oss << value[i];
        }
    }
    return oss.str();
}
