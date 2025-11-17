#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_set>

Lexer::Lexer(const std::string& input) : input_(input) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    std::unordered_set<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", "SELECT", "FROM", "WHERE"
    };

    while (position_ < input_.size()) {
        char current = input_[position_];

        // Skip whitespace
        if (isspace(current)) {
            position_++;
            continue;
        }

        // Handle symbols
        if (current == '(' || current == ')' || current == ',' || current == ';' || current == '*' || current == '=') {
            tokens.push_back({Token::Type::SYMBOL, std::string(1, current)});
            position_++;
            continue;
        }

        // Handle string literals
        if (current == '"') {
            position_++;
            std::string value;
            while (position_ < input_.size() && input_[position_] != '"') {
                value += input_[position_++];
            }
            if (position_ >= input_.size()) {
                throw std::runtime_error("Unterminated string literal");
            }
            position_++;
            tokens.push_back({Token::Type::STRING, value});
            continue;
        }

        // Handle identifiers and keywords
        if (isalpha(current) || current == '_') {
            std::string value;
            while (position_ < input_.size() && (isalnum(input_[position_]) || input_[position_] == '_')) {
                value += toupper(input_[position_++]);
            }

            Token::Type type = keywords.count(value) ? Token::Type::KEYWORD : Token::Type::IDENTIFIER;
            tokens.push_back({type, value});
            continue;
        }

        // Handle numbers and other values
        if (isdigit(current) || current == '-' || current == '+') {
            std::string value;
            while (position_ < input_.size() && (isdigit(input_[position_]) || input_[position_] == '.')) {
                value += input_[position_++];
            }
            tokens.push_back({Token::Type::NUMBER, value});
            continue;
        }

        // If we get here, it's an unrecognized character
        throw std::runtime_error(std::string("Unexpected character: ") + current);
    }

    tokens.push_back({Token::Type::END, ""});
    return tokens;
}
