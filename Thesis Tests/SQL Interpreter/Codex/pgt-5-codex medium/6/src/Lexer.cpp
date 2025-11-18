#include "Lexer.h"

#include <cctype>
#include <stdexcept>
#include <unordered_map>

#include "Utils.h"

namespace {

const std::unordered_map<std::string, TokenType> kKeywordMap = {
    {"CREATE", TokenType::KEYWORD_CREATE},
    {"TABLE", TokenType::KEYWORD_TABLE},
    {"INSERT", TokenType::KEYWORD_INSERT},
    {"INTO", TokenType::KEYWORD_INTO},
    {"VALUES", TokenType::KEYWORD_VALUES},
    {"SELECT", TokenType::KEYWORD_SELECT},
    {"FROM", TokenType::KEYWORD_FROM},
    {"WHERE", TokenType::KEYWORD_WHERE},
};

} // namespace

Lexer::Lexer(const std::string &source) : source_(source) {}

char Lexer::peek() const {
    if (position_ >= source_.size()) {
        return '\0';
    }
    return source_[position_];
}

char Lexer::get() {
    if (position_ >= source_.size()) {
        return '\0';
    }
    return source_[position_++];
}

void Lexer::skipWhitespace() {
    while (true) {
        while (std::isspace(static_cast<unsigned char>(peek()))) {
            get();
        }
        if (peek() == '-' && position_ + 1 < source_.size() && source_[position_ + 1] == '-') {
            while (peek() != '\n' && peek() != '\0') {
                get();
            }
            continue;
        }
        break;
    }
}

Token Lexer::identifierOrKeyword() {
    std::string text;
    char c = peek();
    while (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
        text.push_back(get());
        c = peek();
    }

    std::string upper = Utils::toUpper(text);
    auto it = kKeywordMap.find(upper);
    if (it != kKeywordMap.end()) {
        return Token{it->second, upper};
    }
    return Token{TokenType::IDENTIFIER, text};
}

Token Lexer::number() {
    std::string text;
    char c = peek();
    while (std::isdigit(static_cast<unsigned char>(c))) {
        text.push_back(get());
        c = peek();
    }
    return Token{TokenType::NUMBER, text};
}

Token Lexer::stringLiteral() {
    char quote = get(); // consume opening quote
    (void)quote;
    std::string result;
    while (true) {
        char c = get();
        if (c == '\0') {
            throw std::runtime_error("Unterminated string literal");
        }
        if (c == '\'') {
            if (peek() == '\'') {
                get();
                result.push_back('\'');
                continue;
            }
            break;
        }
        result.push_back(c);
    }
    return Token{TokenType::STRING, result};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespace();
        char c = peek();
        if (c == '\0') {
            break;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(identifierOrKeyword());
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(number());
            continue;
        }
        switch (c) {
            case '\'':
                tokens.push_back(stringLiteral());
                break;
            case '*':
                get();
                tokens.push_back(Token{TokenType::STAR, "*"});
                break;
            case ',':
                get();
                tokens.push_back(Token{TokenType::COMMA, ","});
                break;
            case ';':
                get();
                tokens.push_back(Token{TokenType::SEMICOLON, ";"});
                break;
            case '(':
                get();
                tokens.push_back(Token{TokenType::LPAREN, "("});
                break;
            case ')':
                get();
                tokens.push_back(Token{TokenType::RPAREN, ")"});
                break;
            case '=':
                get();
                tokens.push_back(Token{TokenType::EQUAL, "="});
                break;
            default:
                throw std::runtime_error(std::string("Unexpected character: ") + c);
        }
    }
    tokens.push_back(Token{TokenType::END, ""});
    return tokens;
}
