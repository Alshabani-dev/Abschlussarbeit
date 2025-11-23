#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <unordered_map>

Lexer::Lexer(const std::string& input)
    : input_(input), position_(0), line_(1), column_(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (position_ < input_.length()) {
        skipWhitespace();
        
        if (position_ >= input_.length()) {
            break;
        }
        
        char c = current();
        
        // Single character tokens
        if (c == '(') {
            tokens.emplace_back(TokenType::LEFT_PAREN, "(", line_, column_);
            advance();
        } else if (c == ')') {
            tokens.emplace_back(TokenType::RIGHT_PAREN, ")", line_, column_);
            advance();
        } else if (c == ',') {
            tokens.emplace_back(TokenType::COMMA, ",", line_, column_);
            advance();
        } else if (c == ';') {
            tokens.emplace_back(TokenType::SEMICOLON, ";", line_, column_);
            advance();
        } else if (c == '*') {
            tokens.emplace_back(TokenType::ASTERISK, "*", line_, column_);
            advance();
        } else if (c == '=') {
            tokens.emplace_back(TokenType::EQUALS, "=", line_, column_);
            advance();
        } else if (c == '"' || c == '\'') {
            tokens.push_back(readStringLiteral());
        } else if (isDigit(c)) {
            tokens.push_back(readNumber());
        } else if (isAlpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else {
            error_ = "Unexpected character: " + std::string(1, c);
            tokens.emplace_back(TokenType::INVALID, std::string(1, c), line_, column_);
            advance();
        }
    }
    
    tokens.emplace_back(TokenType::END_OF_FILE, "", line_, column_);
    return tokens;
}

char Lexer::current() const {
    if (position_ >= input_.length()) {
        return '\0';
    }
    return input_[position_];
}

char Lexer::peek(int offset) const {
    size_t pos = position_ + offset;
    if (pos >= input_.length()) {
        return '\0';
    }
    return input_[pos];
}

void Lexer::advance() {
    if (position_ < input_.length()) {
        if (input_[position_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        position_++;
    }
}

void Lexer::skipWhitespace() {
    while (position_ < input_.length() && std::isspace(current())) {
        advance();
    }
}

Token Lexer::readIdentifierOrKeyword() {
    int startColumn = column_;
    std::string text;
    
    while (isAlphaNumeric(current()) || current() == '_') {
        text += current();
        advance();
    }
    
    TokenType type = identifierType(text);
    return Token(type, text, line_, startColumn);
}

Token Lexer::readStringLiteral() {
    int startColumn = column_;
    char quote = current();
    advance(); // Skip opening quote
    
    std::string text;
    while (current() != '\0' && current() != quote) {
        if (current() == '\\' && peek() == quote) {
            // Escaped quote
            advance();
            text += current();
            advance();
        } else {
            text += current();
            advance();
        }
    }
    
    if (current() == quote) {
        advance(); // Skip closing quote
    } else {
        error_ = "Unterminated string literal";
    }
    
    return Token(TokenType::STRING_LITERAL, text, line_, startColumn);
}

Token Lexer::readNumber() {
    int startColumn = column_;
    std::string text;
    
    while (isDigit(current()) || current() == '.') {
        text += current();
        advance();
    }
    
    return Token(TokenType::NUMBER, text, line_, startColumn);
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

TokenType Lexer::identifierType(const std::string& text) const {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"CREATE", TokenType::CREATE},
        {"TABLE", TokenType::TABLE},
        {"INSERT", TokenType::INSERT},
        {"INTO", TokenType::INTO},
        {"VALUES", TokenType::VALUES},
        {"SELECT", TokenType::SELECT},
        {"FROM", TokenType::FROM},
        {"WHERE", TokenType::WHERE}
    };
    
    std::string upper = Utils::toUpper(text);
    auto it = keywords.find(upper);
    if (it != keywords.end()) {
        return it->second;
    }
    
    return TokenType::IDENTIFIER;
}
