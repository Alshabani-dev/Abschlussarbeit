#include "Lexer.h"

#include <cctype>
#include <stdexcept>

#include "Utils.h"

namespace {
bool isIdentifierStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
}

Lexer::Lexer(const std::string &input) : input_(input) {
    tokenize();
}

const Token &Lexer::peek(size_t offset) const {
    size_t index = position_ + offset;
    if (index >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[index];
}

Token Lexer::consume() {
    if (position_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[position_++];
}

void Lexer::tokenize() {
    tokens_.clear();
    size_t i = 0;
    while (i < input_.size()) {
        char c = input_[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }

        if (isIdentifierStart(c)) {
            size_t start = i;
            ++i;
            while (i < input_.size() && Utils::isIdentifierChar(input_[i])) {
                ++i;
            }
            tokens_.push_back({TokenType::Identifier, input_.substr(start, i - start)});
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            size_t start = i;
            ++i;
            while (i < input_.size() && std::isdigit(static_cast<unsigned char>(input_[i]))) {
                ++i;
            }
            tokens_.push_back({TokenType::Number, input_.substr(start, i - start)});
            continue;
        }

        if (c == '\'' || c == '"') {
            char quote = c;
            ++i;
            size_t start = i;
            std::string value;
            bool closed = false;
            while (i < input_.size()) {
                char current = input_[i];
                if (current == '\\' && i + 1 < input_.size()) {
                    value.push_back(input_[i + 1]);
                    i += 2;
                    continue;
                }
                if (current == quote) {
                    closed = true;
                    ++i;
                    break;
                }
                value.push_back(current);
                ++i;
            }
            if (!closed) {
                throw std::runtime_error("Unterminated string literal");
            }
            tokens_.push_back({TokenType::StringLiteral, value});
            continue;
        }

        switch (c) {
            case ',':
                tokens_.push_back({TokenType::Comma, ","});
                ++i;
                break;
            case '(':
                tokens_.push_back({TokenType::LParen, "("});
                ++i;
                break;
            case ')':
                tokens_.push_back({TokenType::RParen, ")"});
                ++i;
                break;
            case ';':
                tokens_.push_back({TokenType::Semicolon, ";"});
                ++i;
                break;
            case '*':
                tokens_.push_back({TokenType::Star, "*"});
                ++i;
                break;
            case '=':
                tokens_.push_back({TokenType::Equals, "="});
                ++i;
                break;
            default:
                throw std::runtime_error("Unexpected character in input");
        }
    }

    tokens_.push_back({TokenType::End, ""});
}
