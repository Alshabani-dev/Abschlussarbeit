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
        while (start != s.end() && std::isspace(*start)) {
            start++;
        }

        auto end = s.end();
        do {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(*end));

        return std::string(start, end + 1);
    }

    std::string toLower(const std::string &s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::string escapeCsvField(const std::string &field) {
        std::string result;
        for (char c : field) {
            if (c == '"') {
                result += "\"\"";
            } else if (c == ',' || c == '\n' || c == '\r') {
                result += '"' + std::string(1, c) + '"';
            } else {
                result += c;
            }
        }

        // If the field contains quotes or commas, wrap it in quotes
        if (field.find('"') != std::string::npos ||
            field.find(',') != std::string::npos ||
            field.find('\n') != std::string::npos) {
            return '"' + result + '"';
        }

        return result;
    }

    std::string unescapeCsvField(const std::string &field) {
        std::string result;
        bool inQuotes = false;

        for (size_t i = 0; i < field.size(); i++) {
            char c = field[i];

            if (c == '"') {
                if (inQuotes && i + 1 < field.size() && field[i + 1] == '"') {
                    // Escaped quote
                    result += '"';
                    i++; // Skip next quote
                } else {
                    // Start or end of quoted field
                    inQuotes = !inQuotes;
                }
            } else {
                result += c;
            }
        }

        return result;
    }
}
