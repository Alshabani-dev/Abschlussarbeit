#include "Lexer.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "Utils.h"

namespace {

TokenType keywordType(const std::string &upper) {
    if (upper == "CREATE") return TokenType::KEYWORD_CREATE;
    if (upper == "TABLE") return TokenType::KEYWORD_TABLE;
    if (upper == "INSERT") return TokenType::KEYWORD_INSERT;
    if (upper == "INTO") return TokenType::KEYWORD_INTO;
    if (upper == "VALUES") return TokenType::KEYWORD_VALUES;
    if (upper == "SELECT") return TokenType::KEYWORD_SELECT;
    if (upper == "FROM") return TokenType::KEYWORD_FROM;
    if (upper == "WHERE") return TokenType::KEYWORD_WHERE;
    return TokenType::IDENTIFIER;
}

bool isIdentifierChar(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-';
}

} // namespace

Lexer::Lexer(const std::string &input) : input_(input), position_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!atEnd()) {
        skipWhitespace();
        if (atEnd()) {
            break;
        }

        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(parseIdentifier());
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(parseIdentifier());
        } else if (c == '"'
                   || c == '\'') {
            tokens.push_back(parseStringLiteral());
        } else {
            switch (c) {
                case '*':
                    tokens.push_back({TokenType::STAR, std::string(1, advance())});
                    break;
                case ',':
                    tokens.push_back({TokenType::COMMA, std::string(1, advance())});
                    break;
                case '(':
                    tokens.push_back({TokenType::LPAREN, std::string(1, advance())});
                    break;
                case ')':
                    tokens.push_back({TokenType::RPAREN, std::string(1, advance())});
                    break;
                case ';':
                    tokens.push_back({TokenType::SEMICOLON, std::string(1, advance())});
                    break;
                case '=':
                    tokens.push_back({TokenType::EQUALS, std::string(1, advance())});
                    break;
                default:
                    throw std::runtime_error("Unexpected character in input: " + std::string(1, c));
            }
        }
    }
    tokens.push_back({TokenType::END, ""});
    return tokens;
}

bool Lexer::atEnd() const {
    return position_ >= input_.size();
}

char Lexer::peek() const {
    return input_[position_];
}

char Lexer::advance() {
    return input_[position_++];
}

void Lexer::skipWhitespace() {
    while (!atEnd()) {
        char c = peek();
        if (c == '-' && position_ + 1 < input_.size() && input_[position_ + 1] == '-') {
            while (!atEnd() && peek() != '\n') {
                advance();
            }
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
        } else {
            break;
        }
    }
}

Token Lexer::parseIdentifier() {
    std::size_t start = position_;
    while (!atEnd() && isIdentifierChar(peek())) {
        advance();
    }
    std::string text = input_.substr(start, position_ - start);
    std::string upper = Utils::toUpper(text);
    TokenType type = keywordType(upper);
    if (type == TokenType::IDENTIFIER) {
        return {type, text};
    }
    return {type, upper};
}

Token Lexer::parseStringLiteral() {
    char quote = advance();
    std::string value;
    bool closed = false;
    while (!atEnd()) {
        char c = advance();
        if (c == quote) {
            closed = true;
            break;
        }
        if (c == '\\' && !atEnd()) {
            char next = advance();
            value.push_back(next);
        } else {
            value.push_back(c);
        }
    }
    if (!closed) {
        throw std::runtime_error("Unterminated string literal");
    }
    return {TokenType::STRING_LITERAL, value};
}
