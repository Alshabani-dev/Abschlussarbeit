#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

Lexer::Lexer(const std::string& input) : input_(input) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    std::unordered_map<std::string, TokenType> keywords = {
        {"CREATE", TokenType::CREATE},
        {"TABLE", TokenType::TABLE},
        {"INSERT", TokenType::INSERT},
        {"INTO", TokenType::INTO},
        {"VALUES", TokenType::VALUES},
        {"SELECT", TokenType::SELECT},
        {"FROM", TokenType::FROM},
        {"WHERE", TokenType::WHERE}
    };

    while (position_ < input_.size()) {
        char current = input_[position_];

        // Skip whitespace
        if (isspace(current)) {
            position_++;
            continue;
        }

        // Handle symbols
        if (current == '(') {
            tokens.push_back({TokenType::LPAREN, "("});
            position_++;
        } else if (current == ')') {
            tokens.push_back({TokenType::RPAREN, ")"});
            position_++;
        } else if (current == ',') {
            tokens.push_back({TokenType::COMMA, ","});
            position_++;
        } else if (current == ';') {
            tokens.push_back({TokenType::SEMICOLON, ";"});
            position_++;
        } else if (current == '*') {
            tokens.push_back({TokenType::ASTERISK, "*"});
            position_++;
        } else if (current == '=') {
            tokens.push_back({TokenType::EQUALS, "="});
            position_++;
        }
        // Handle identifiers and keywords
        else if (isalpha(current) || current == '_') {
            std::string value;
            while (position_ < input_.size() && (isalnum(input_[position_]) || input_[position_] == '_')) {
                value += input_[position_];
                position_++;
            }

            // Convert to uppercase for keyword comparison
            std::string upperValue;
            for (char c : value) {
                upperValue += toupper(c);
            }

            // Check if it's a keyword
            if (keywords.find(upperValue) != keywords.end()) {
                tokens.push_back({keywords[upperValue], upperValue});
            } else {
                tokens.push_back({TokenType::IDENTIFIER, value});
            }
        }
        // Handle string literals
        else if (current == '"') {
            position_++;
            std::string value;
            while (position_ < input_.size() && input_[position_] != '"') {
                value += input_[position_];
                position_++;
            }
            if (position_ < input_.size()) {
                position_++; // Skip closing quote
            }
            tokens.push_back({TokenType::STRING_LITERAL, value});
        }
        // Handle numbers
        else if (isdigit(current)) {
            std::string value;
            while (position_ < input_.size() && isdigit(input_[position_])) {
                value += input_[position_];
                position_++;
            }
            tokens.push_back({TokenType::NUMBER, value});
        }
        // Unknown character - skip it instead of throwing an error
        else {
            position_++;
        }
    }

    tokens.push_back({TokenType::END, ""});
    return tokens;
}
