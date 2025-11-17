#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Utils {
    std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string trim(const std::string& s) {
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

    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::string escapeCsv(const std::string& s) {
        if (s.find(',') == std::string::npos && s.find('"') == std::string::npos && s.find('\n') == std::string::npos) {
            return s;
        }
        std::string result = "\"";
        for (char c : s) {
            if (c == '"') {
                result += "\"\"";
            } else {
                result += c;
            }
        }
        result += "\"";
        return result;
    }

    std::string unescapeCsv(const std::string& s) {
        if (s.empty() || s.front() != '"' || s.back() != '"') {
            return s;
        }
        std::string result;
        for (size_t i = 1; i < s.size() - 1; i++) {
            if (s[i] == '"' && s[i+1] == '"') {
                result += '"';
                i++;
            } else {
                result += s[i];
            }
        }
        return result;
    }
}
