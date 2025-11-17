#include "Lexer.h"
#include <cctype>
#include <unordered_set>

Lexer::Lexer(const std::string& input) : input_(input), position_(0) {}

char Lexer::peek() const {
    if (position_ >= input_.size()) {
        return '\0';
    }
    return input_[position_];
}

char Lexer::consume() {
    return input_[position_++];
}

void Lexer::skipWhitespace() {
    while (isspace(peek())) {
        consume();
    }
}

Token Lexer::readIdentifierOrKeyword() {
    std::string value;
    size_t startPos = position_;

    while (isalnum(peek()) || peek() == '_') {
        value += consume();
    }

    // Check if it's a keyword
    static const std::unordered_set<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", "SELECT", "FROM", "WHERE"
    };

    TokenType type = TokenType::IDENTIFIER;
    if (keywords.find(value) != keywords.end()) {
        type = TokenType::KEYWORD;
    }

    return {type, value, startPos};
}

Token Lexer::readStringLiteral() {
    size_t startPos = position_;
    std::string value;

    // Skip opening quote
    consume();

    while (peek() != '"' && peek() != '\0') {
        value += consume();
    }

    // Skip closing quote
    if (peek() == '"') {
        consume();
    }

    return {TokenType::STRING_LITERAL, value, startPos};
}

Token Lexer::readNumber() {
    size_t startPos = position_;
    std::string value;

    while (isdigit(peek())) {
        value += consume();
    }

    return {TokenType::NUMBER, value, startPos};
}

Token Lexer::readSymbol() {
    size_t startPos = position_;
    std::string value;

    // Read single character symbol
    value += consume();

    return {TokenType::SYMBOL, value, startPos};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (position_ < input_.size()) {
        skipWhitespace();

        if (position_ >= input_.size()) {
            break;
        }

        char c = peek();

        if (isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else if (c == '"') {
            tokens.push_back(readStringLiteral());
        } else if (isdigit(c)) {
            tokens.push_back(readNumber());
        } else {
            tokens.push_back(readSymbol());
        }
    }

    tokens.push_back({TokenType::END_OF_INPUT, "", position_});
    return tokens;
}
