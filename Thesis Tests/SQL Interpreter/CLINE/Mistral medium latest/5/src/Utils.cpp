#include "Utils.h"
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
        [](unsigned char c){ return std::tolower(c); });
    return result;
}

std::string Utils::join(const std::vector<std::string> &strings, const std::string &delimiter) {
    std::string result;
    for (size_t i = 0; i < strings.size(); i++) {
        if (i != 0) {
            result += delimiter;
        }
        result += strings[i];
    }
    return result;
}

std::string Utils::escapeCsvField(const std::string &field) {
    if (field.find(',') == std::string::npos &&
        field.find('"') == std::string::npos &&
        field.find('\n') == std::string::npos) {
        return field;
    }

    std::string escaped;
    escaped += '"';
    for (char c : field) {
        if (c == '"') {
            escaped += '"'; // Double the quote
        }
        escaped += c;
    }
    escaped += '"';
    return escaped;
}

std::string Utils::unescapeCsvField(const std::string &field) {
    if (field.empty() || field.front() != '"' || field.back() != '"') {
        return field;
    }

    std::string unescaped;
    bool inQuotes = false;
    for (size_t i = 0; i < field.size(); i++) {
        if (i == 0 || i == field.size() - 1) {
            continue; // Skip outer quotes
        }
        if (field[i] == '"' && field[i-1] == '"') {
            continue; // Skip escaped quote
        }
        unescaped += field[i];
    }
    return unescaped;
}
