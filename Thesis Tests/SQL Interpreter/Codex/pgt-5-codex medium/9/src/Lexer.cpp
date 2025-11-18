#include "Lexer.h"

#include <cctype>
#include <unordered_map>

#include "Utils.h"

namespace {

TokenType keywordType(const std::string &value) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"CREATE", TokenType::KEYWORD_CREATE},
        {"TABLE", TokenType::KEYWORD_TABLE},
        {"INSERT", TokenType::KEYWORD_INSERT},
        {"INTO", TokenType::KEYWORD_INTO},
        {"VALUES", TokenType::KEYWORD_VALUES},
        {"SELECT", TokenType::KEYWORD_SELECT},
        {"FROM", TokenType::KEYWORD_FROM},
        {"WHERE", TokenType::KEYWORD_WHERE}
    };

    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::IDENTIFIER;
}

} // namespace

Lexer::Lexer(const std::string &input) : input_(input), position_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!eof()) {
        skipWhitespace();
        if (eof()) {
            break;
        }

        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(identifier());
        } else if (c == '\'' || c == '"') {
            Token token = stringLiteral();
            if (!error_.empty()) {
                return {};
            }
            tokens.push_back(token);
        } else {
            switch (c) {
            case ',':
                tokens.push_back({TokenType::COMMA, std::string(1, c)});
                advance();
                break;
            case '(':
                tokens.push_back({TokenType::LPAREN, std::string(1, c)});
                advance();
                break;
            case ')':
                tokens.push_back({TokenType::RPAREN, std::string(1, c)});
                advance();
                break;
            case '*':
                tokens.push_back({TokenType::STAR, std::string(1, c)});
                advance();
                break;
            case '=':
                tokens.push_back({TokenType::EQUAL, std::string(1, c)});
                advance();
                break;
            case ';':
                // Ignore semicolons here; they are handled by statement splitting.
                advance();
                break;
            default:
                error_ = "Unexpected character: " + std::string(1, c);
                return {};
            }
        }
    }
    tokens.push_back({TokenType::END, ""});
    return tokens;
}

std::string Lexer::getError() const {
    return error_;
}

void Lexer::skipWhitespace() {
    while (!eof() && std::isspace(static_cast<unsigned char>(peek()))) {
        advance();
    }
}

bool Lexer::eof() const {
    return position_ >= input_.size();
}

char Lexer::peek() const {
    return input_[position_];
}

char Lexer::advance() {
    return input_[position_++];
}

Token Lexer::identifier() {
    size_t start = position_;
    while (!eof()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            advance();
        } else {
            break;
        }
    }
    std::string text = input_.substr(start, position_ - start);
    std::string upper = Utils::toUpper(text);
    TokenType type = keywordType(upper);
    if (type == TokenType::IDENTIFIER) {
        return {type, text};
    }
    return {type, upper};
}

Token Lexer::stringLiteral() {
    char quote = advance();
    std::string value;
    while (!eof()) {
        char c = advance();
        if (c == quote) {
            if (!eof() && peek() == quote) {
                // Escaped quote represented by doubling it.
                value.push_back(quote);
                advance();
                continue;
            }
            return {TokenType::STRING_LITERAL, value};
        }
        if (c == '\\' && !eof()) {
            value.push_back(advance());
        } else {
            value.push_back(c);
        }
    }
    error_ = "Unterminated string literal";
    return {TokenType::END, ""};
}
