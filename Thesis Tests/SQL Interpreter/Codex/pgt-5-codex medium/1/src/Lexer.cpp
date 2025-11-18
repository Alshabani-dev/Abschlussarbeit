#include "Lexer.h"

#include <cctype>
#include <stdexcept>

#include "Utils.h"

namespace {
const std::unordered_map<std::string, Token::Type> kKeywords = {
    {"CREATE", Token::Type::Create}, {"TABLE", Token::Type::Table},
    {"INSERT", Token::Type::Insert}, {"INTO", Token::Type::Into},
    {"VALUES", Token::Type::Values}, {"SELECT", Token::Type::Select},
    {"FROM", Token::Type::From},     {"WHERE", Token::Type::Where}};
} // namespace

Lexer::Lexer(std::string input) : input_(std::move(input)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!eof()) {
        skipWhitespace();
        if (eof()) {
            break;
        }
        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.emplace_back(identifierOrKeyword());
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.emplace_back(number());
            continue;
        }
        switch (c) {
        case ',':
            advance();
            tokens.push_back({Token::Type::Comma, ","});
            break;
        case ';':
            advance();
            tokens.push_back({Token::Type::Semicolon, ";"});
            break;
        case '(':
            advance();
            tokens.push_back({Token::Type::LParen, "("});
            break;
        case ')':
            advance();
            tokens.push_back({Token::Type::RParen, ")"});
            break;
        case '*':
            advance();
            tokens.push_back({Token::Type::Asterisk, "*"});
            break;
        case '=':
            advance();
            tokens.push_back({Token::Type::Equal, "="});
            break;
        case '\'':
        case '"':
            tokens.emplace_back(stringLiteral());
            break;
        default:
            throw std::runtime_error(std::string("Unexpected character: ") + c);
        }
    }
    tokens.push_back({Token::Type::EndOfFile, ""});
    return tokens;
}

char Lexer::peek() const {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_];
}

char Lexer::advance() {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_++];
}

bool Lexer::eof() const { return pos_ >= input_.size(); }

void Lexer::skipWhitespace() {
    while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
        advance();
    }
}

Token Lexer::identifierOrKeyword() {
    std::string value;
    while (!eof()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            value.push_back(c);
            advance();
        } else {
            break;
        }
    }
    std::string upper = utils::toUpper(value);
    auto keywordIt = kKeywords.find(upper);
    if (keywordIt != kKeywords.end()) {
        return Token{keywordIt->second, upper};
    }
    return Token{Token::Type::Identifier, value};
}

Token Lexer::number() {
    std::string value;
    while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
        value.push_back(advance());
    }
    return Token{Token::Type::Number, value};
}

Token Lexer::stringLiteral() {
    char quote = advance();
    std::string value;
    while (!eof()) {
        char c = advance();
        if (c == quote) {
            break;
        }
        if (c == '\\' && !eof()) {
            char next = advance();
            value.push_back(next);
            continue;
        }
        value.push_back(c);
    }
    return Token{Token::Type::String, value};
}
