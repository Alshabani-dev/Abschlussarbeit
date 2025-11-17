#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <unordered_set>

Lexer::Lexer(const std::string &input) : input_(input), position_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (position_ < input_.size()) {
        skipWhitespace();
        if (position_ >= input_.size()) break;

        Token token = readNextToken();
        if (token.type != TokenType::END_OF_INPUT) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

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
    while (position_ < input_.size() && std::isspace(input_[position_])) {
        position_++;
    }
}

Token Lexer::readNextToken() {
    char c = peek();

    if (std::isalpha(c) || c == '_') {
        return readIdentifierOrKeyword();
    }
    else if (c == '"' || c == '\'') {
        return readStringLiteral();
    }
    else if (std::isdigit(c)) {
        return readNumber();
    }
    else if (c == '\0') {
        return {TokenType::END_OF_INPUT, "", position_};
    }
    else {
        return readSymbol();
    }
}

Token Lexer::readIdentifierOrKeyword() {
    std::string value;
    size_t startPos = position_;

    while (position_ < input_.size() && (std::isalnum(peek()) || peek() == '_')) {
        value += consume();
    }

    // Check if it's a keyword
    static const std::unordered_set<std::string> keywords = {
        "create", "table", "insert", "into", "values", "select", "from", "where", "and", "or"
    };

    TokenType type = TokenType::IDENTIFIER;
    if (keywords.find(Utils::toLower(value)) != keywords.end()) {
        type = TokenType::KEYWORD;
    }

    return {type, value, startPos};
}

Token Lexer::readStringLiteral() {
    char quote = consume(); // Opening quote
    std::string value;
    size_t startPos = position_;

    while (position_ < input_.size() && peek() != quote) {
        if (peek() == '\n') {
            // Handle unterminated string
            break;
        }
        value += consume();
    }

    if (position_ < input_.size()) {
        consume(); // Closing quote
    }

    return {TokenType::STRING_LITERAL, value, startPos};
}

Token Lexer::readNumber() {
    std::string value;
    size_t startPos = position_;

    while (position_ < input_.size() && std::isdigit(peek())) {
        value += consume();
    }

    return {TokenType::NUMBER, value, startPos};
}

Token Lexer::readSymbol() {
    char c = consume();
    return {TokenType::SYMBOL, std::string(1, c), position_ - 1};
}
