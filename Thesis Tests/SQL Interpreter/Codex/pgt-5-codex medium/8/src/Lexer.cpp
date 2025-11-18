#include "Lexer.h"

#include <cctype>

#include "Utils.h"

namespace {
const std::unordered_set<std::string> kKeywords = {
    "CREATE", "TABLE", "INSERT", "INTO", "VALUES", "SELECT", "FROM", "WHERE"
};
}

Lexer::Lexer(const std::string &input) : input_(input) {}

char Lexer::current() const {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_];
}

void Lexer::advance() {
    if (pos_ < input_.size()) {
        ++pos_;
    }
}

void Lexer::skipWhitespace() {
    while (std::isspace(static_cast<unsigned char>(current()))) {
        advance();
    }
}

Token Lexer::makeKeywordOrIdentifier(const std::string &value) const {
    std::string upper = utils::toUpper(value);
    if (kKeywords.count(upper)) {
        return Token{TokenType::Keyword, upper};
    }
    return Token{TokenType::Identifier, value};
}

Token Lexer::readIdentifier() {
    size_t start = pos_;
    while (std::isalnum(static_cast<unsigned char>(current())) || current() == '_') {
        advance();
    }
    return makeKeywordOrIdentifier(input_.substr(start, pos_ - start));
}

Token Lexer::readNumber() {
    size_t start = pos_;
    while (std::isdigit(static_cast<unsigned char>(current()))) {
        advance();
    }
    return Token{TokenType::Number, input_.substr(start, pos_ - start)};
}

Token Lexer::readString() {
    advance(); // skip opening quote
    std::string value;
    while (true) {
        char c = current();
        if (c == '\0') {
            break;
        }
        if (c == '\'') {
            advance();
            if (current() == '\'') {
                value += '\'';
                advance();
            } else {
                break;
            }
        } else {
            value += c;
            advance();
        }
    }
    return Token{TokenType::String, value};
}

Token Lexer::next() {
    if (hasLookahead_) {
        hasLookahead_ = false;
        return lookahead_;
    }

    skipWhitespace();
    char c = current();
    if (c == '\0') {
        return Token{TokenType::End, ""};
    }

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return readIdentifier();
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        return readNumber();
    }

    if (c == '\'') {
        return readString();
    }

    advance();
    switch (c) {
        case ',':
            return Token{TokenType::Comma, ","};
        case '(':
            return Token{TokenType::LParen, "("};
        case ')':
            return Token{TokenType::RParen, ")"};
        case '*':
            return Token{TokenType::Star, "*"};
        case '=':
            return Token{TokenType::Equal, "="};
        case ';':
            return Token{TokenType::Semicolon, ";"};
        default:
            return Token{TokenType::Unknown, std::string(1, c)};
    }
}

Token Lexer::peek() {
    if (!hasLookahead_) {
        lookahead_ = next();
        hasLookahead_ = true;
    }
    return lookahead_;
}
