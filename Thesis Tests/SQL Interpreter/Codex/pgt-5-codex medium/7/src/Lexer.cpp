#include "Lexer.h"

#include <stdexcept>

#include "Utils.h"

namespace {

bool isIdentifierStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

TokenType keywordType(const std::string &upper) {
    if (upper == "CREATE") return TokenType::KeywordCreate;
    if (upper == "TABLE") return TokenType::KeywordTable;
    if (upper == "INSERT") return TokenType::KeywordInsert;
    if (upper == "INTO") return TokenType::KeywordInto;
    if (upper == "VALUES") return TokenType::KeywordValues;
    if (upper == "SELECT") return TokenType::KeywordSelect;
    if (upper == "FROM") return TokenType::KeywordFrom;
    if (upper == "WHERE") return TokenType::KeywordWhere;
    return TokenType::Identifier;
}

} // namespace

Lexer::Lexer(const std::string &input)
    : input_(input), position_(0), hasCurrent_(false) {}

Token Lexer::peek() {
    if (!hasCurrent_) {
        current_ = next();
        hasCurrent_ = true;
    }
    return current_;
}

Token Lexer::next() {
    if (hasCurrent_) {
        hasCurrent_ = false;
        return current_;
    }

    skipWhitespace();
    if (position_ >= input_.size()) {
        return {TokenType::End, ""};
    }

    char c = input_[position_];
    if (isIdentifierStart(c)) {
        return readIdentifierOrKeyword();
    }
    if (isDigit(c)) {
        return readNumber();
    }
    if (c == '\'' || c == '"') {
        return readString();
    }

    ++position_;
    switch (c) {
    case ',':
        return {TokenType::Comma, ","};
    case '(':
        return {TokenType::LParen, "("};
    case ')':
        return {TokenType::RParen, ")"};
    case ';':
        return {TokenType::Semicolon, ";"};
    case '*':
        return {TokenType::Star, "*"};
    case '=':
        return {TokenType::Equal, "="};
    default:
        throw std::runtime_error(std::string("Unexpected character: ") + c);
    }
}

void Lexer::skipWhitespace() {
    while (position_ < input_.size()) {
        char c = input_[position_];
        if (c == '-' && position_ + 1 < input_.size() && input_[position_ + 1] == '-') {
            position_ += 2;
            while (position_ < input_.size() && input_[position_] != '\n') {
                ++position_;
            }
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            ++position_;
        } else {
            break;
        }
    }
}

Token Lexer::readIdentifierOrKeyword() {
    size_t start = position_;
    ++position_;
    while (position_ < input_.size() && isIdentifierChar(input_[position_])) {
        ++position_;
    }
    std::string word = input_.substr(start, position_ - start);
    std::string upper = toUpper(word);
    TokenType type = keywordType(upper);
    if (type == TokenType::Identifier) {
        return {type, word};
    }
    return {type, upper};
}

Token Lexer::readNumber() {
    size_t start = position_;
    while (position_ < input_.size() && isDigit(input_[position_])) {
        ++position_;
    }
    return {TokenType::Number, input_.substr(start, position_ - start)};
}

Token Lexer::readString() {
    char quote = input_[position_];
    ++position_;
    std::string value;
    while (position_ < input_.size()) {
        char c = input_[position_++];
        if (c == quote) {
            if (position_ < input_.size() && input_[position_] == quote) {
                value.push_back(quote);
                ++position_;
            } else {
                break;
            }
        } else {
            value.push_back(c);
        }
    }
    return {TokenType::String, value};
}
