#include "Utils.h"
#include <algorithm>
#include <sstream>

namespace Utils {
    std::vector<std::string> split(const std::string &s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string trim(const std::string &s) {
        auto start = s.begin();
        while (start != s.end() && isspace(*start)) {
            start++;
        }
        auto end = s.end();
        do {
            end--;
        } while (std::distance(start, end) > 0 && isspace(*end));
        return std::string(start, end + 1);
    }

    std::string toLower(const std::string &s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::string join(const std::vector<std::string> &strings, const std::string &delimiter) {
        std::ostringstream oss;
        for (size_t i = 0; i < strings.size(); ++i) {
            if (i != 0) {
                oss << delimiter;
            }
            oss << strings[i];
        }
        return oss.str();
    }

    std::string escapeCsv(const std::string &value) {
        if (value.find_first_of("\",\n") == std::string::npos) {
            return value;
        }

        std::ostringstream oss;
        oss << "\"";
        for (char c : value) {
            if (c == '"') {
                oss << "\"\"";
            } else {
                oss << c;
            }
        }
        oss << "\"";
        return oss.str();
    }

    std::string unescapeCsv(const std::string &value) {
        if (value.empty() || value.front() != '"') {
            return value;
        }

        std::ostringstream oss;
        bool escaped = false;
        for (size_t i = 1; i < value.size() - 1; ++i) {
            char c = value[i];
            if (escaped) {
                oss << c;
                escaped = false;
            } else if (c == '"') {
                escaped = true;
            } else {
                oss << c;
            }
        }
        return oss.str();
    }
}
