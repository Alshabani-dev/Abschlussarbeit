#include "Lexer.h"

#include <cctype>

#include "Utils.h"

Lexer::Lexer(const std::string &input) : input_(input) {}

char Lexer::peek() const {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_];
}

char Lexer::get() {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_++];
}

void Lexer::skipWhitespace() {
    while (std::isspace(static_cast<unsigned char>(peek()))) {
        ++pos_;
    }
}

Token Lexer::identifier() {
    size_t start = pos_ - 1;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        ++pos_;
    }
    std::string text = input_.substr(start, pos_ - start);
    Token token;
    token.text = text;
    token.type = keywordType(Utils::toUpper(text));
    if (token.type == TokenType::Unknown) {
        token.type = TokenType::Identifier;
    }
    return token;
}

Token Lexer::number() {
    size_t start = pos_ - 1;
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        ++pos_;
    }
    return {TokenType::Number, input_.substr(start, pos_ - start)};
}

Token Lexer::stringLiteral() {
    char quote = input_[pos_ - 1];
    std::string value;
    bool closed = false;
    while (pos_ < input_.size()) {
        char c = get();
        if (c == quote) {
            if (peek() == quote) { // escape double quote
                value.push_back(quote);
                ++pos_;
                continue;
            }
            closed = true;
            break;
        }
        value.push_back(c);
    }
    if (!closed) {
        return {TokenType::Unknown, value};
    }
    return {TokenType::StringLiteral, value};
}

TokenType Lexer::keywordType(const std::string &value) {
    if (value == "CREATE") return TokenType::Create;
    if (value == "TABLE") return TokenType::TableKW;
    if (value == "INSERT") return TokenType::Insert;
    if (value == "INTO") return TokenType::Into;
    if (value == "VALUES") return TokenType::Values;
    if (value == "SELECT") return TokenType::Select;
    if (value == "FROM") return TokenType::From;
    if (value == "WHERE") return TokenType::Where;
    return TokenType::Unknown;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos_ < input_.size()) {
        skipWhitespace();
        char c = peek();
        if (c == '\0') {
            break;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            get();
            tokens.push_back(identifier());
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            get();
            tokens.push_back(number());
        } else if (c == '\'' || c == '"') {
            get();
            tokens.push_back(stringLiteral());
        } else {
            get();
            switch (c) {
                case ',': tokens.push_back({TokenType::Comma, ","}); break;
                case '(': tokens.push_back({TokenType::LParen, "("}); break;
                case ')': tokens.push_back({TokenType::RParen, ")"}); break;
                case ';': tokens.push_back({TokenType::Semicolon, ";"}); break;
                case '*': tokens.push_back({TokenType::Star, "*"}); break;
                case '=': tokens.push_back({TokenType::Equal, "="}); break;
                default: tokens.push_back({TokenType::Unknown, std::string(1, c)}); break;
            }
        }
    }
    tokens.push_back({TokenType::End, ""});
    return tokens;
}
