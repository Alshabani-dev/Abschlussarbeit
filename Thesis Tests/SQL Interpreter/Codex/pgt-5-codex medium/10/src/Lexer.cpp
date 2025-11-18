#include "Lexer.h"

#include <cctype>
#include <stdexcept>

#include "Utils.h"

namespace {

bool isIdentifierStart(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

bool isIdentifierChar(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

bool isKeyword(const std::string &text) {
    static const std::vector<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", "SELECT", "FROM", "WHERE", "AND", "OR"
    };
    auto upper = utils::toUpper(text);
    for (const auto &kw : keywords) {
        if (kw == upper) {
            return true;
        }
    }
    return false;
}

} // namespace

Lexer::Lexer(const std::string &input) : input_(input) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!eof()) {
        skipWhitespace();
        if (eof()) {
            break;
        }
        char ch = peek();
        if (isIdentifierStart(ch)) {
            tokens.push_back(identifier());
        } else if (std::isdigit(static_cast<unsigned char>(ch))) {
            tokens.push_back(number());
        } else if (ch == '\'' || ch == '"') {
            tokens.push_back(stringLiteral());
        } else {
            switch (ch) {
                case ',':
                    get();
                    tokens.push_back({TokenType::Comma, ","});
                    break;
                case '(':
                    get();
                    tokens.push_back({TokenType::LParen, "("});
                    break;
                case ')':
                    get();
                    tokens.push_back({TokenType::RParen, ")"});
                    break;
                case ';':
                    get();
                    tokens.push_back({TokenType::Semicolon, ";"});
                    break;
                case '*':
                    get();
                    tokens.push_back({TokenType::Star, "*"});
                    break;
                case '=':
                    get();
                    tokens.push_back({TokenType::Equal, "="});
                    break;
                default:
                    throw std::runtime_error(std::string("Unexpected character: ") + ch);
            }
        }
    }
    tokens.push_back({TokenType::EndOfFile, ""});
    return tokens;
}

void Lexer::skipWhitespace() {
    while (!eof()) {
        char ch = peek();
        if (std::isspace(static_cast<unsigned char>(ch))) {
            get();
        } else {
            break;
        }
    }
}

char Lexer::peek() const {
    if (position_ >= input_.size()) {
        return '\0';
    }
    return input_[position_];
}

char Lexer::get() {
    if (position_ >= input_.size()) {
        return '\0';
    }
    return input_[position_++];
}

bool Lexer::eof() const {
    return position_ >= input_.size();
}

Token Lexer::identifier() {
    size_t start = position_;
    while (!eof() && isIdentifierChar(peek())) {
        get();
    }
    std::string text = input_.substr(start, position_ - start);
    if (isKeyword(text)) {
        return {TokenType::Keyword, utils::toUpper(text)};
    }
    return {TokenType::Identifier, text};
}

Token Lexer::number() {
    size_t start = position_;
    while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
        get();
    }
    return {TokenType::Number, input_.substr(start, position_ - start)};
}

Token Lexer::stringLiteral() {
    char quote = get();
    std::string value;
    while (!eof()) {
        char ch = get();
        if (ch == quote) {
            break;
        }
        if (ch == '\\' && !eof()) {
            char next = get();
            value.push_back(next);
        } else {
            value.push_back(ch);
        }
    }
    return {TokenType::String, value};
}
