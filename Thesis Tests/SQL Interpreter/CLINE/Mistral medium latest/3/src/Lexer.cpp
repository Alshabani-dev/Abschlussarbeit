#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer() {}

std::vector<Token> Lexer::tokenize(const std::string &input) {
    std::vector<Token> tokens;
    size_t pos = 0;
    size_t length = input.length();

    while (pos < length) {
        char current = input[pos];

        // Skip whitespace
        if (isspace(current)) {
            pos++;
            continue;
        }

        // Handle symbols
        if (current == '(' || current == ')' || current == ',' || current == ';' || current == '=' || current == '*') {
            tokens.push_back({TokenType::SYMBOL, std::string(1, current), pos});
            pos++;
            continue;
        }

        // Handle strings
        if (current == '"' || current == '\'') {
            char quote = current;
            pos++;
            std::string value;
            while (pos < length && input[pos] != quote) {
                value += input[pos];
                pos++;
            }
            if (pos < length) {
                pos++; // Skip closing quote
            }
            tokens.push_back({TokenType::STRING, value, pos - value.length() - 2});
            continue;
        }

        // Handle identifiers and keywords
        if (isalpha(current) || current == '_') {
            std::string value;
            while (pos < length && (isalnum(input[pos]) || input[pos] == '_')) {
                value += input[pos];
                pos++;
            }
            TokenType type = isKeyword(value) ? TokenType::KEYWORD : TokenType::IDENTIFIER;
            tokens.push_back({type, value, pos - value.length()});
            continue;
        }

        // Handle numbers
        if (isdigit(current)) {
            std::string value;
            while (pos < length && isdigit(input[pos])) {
                value += input[pos];
                pos++;
            }
            tokens.push_back({TokenType::NUMBER, value, pos - value.length()});
            continue;
        }

        // Unknown token
        throw std::runtime_error("Unexpected character: " + std::string(1, current));
    }

    tokens.push_back({TokenType::END_OF_INPUT, "", pos});
    return tokens;
}

bool Lexer::isKeyword(const std::string &value) const {
    for (const auto &keyword : keywords_) {
        if (Utils::toLower(value) == Utils::toLower(keyword)) {
            return true;
        }
    }
    return false;
}
