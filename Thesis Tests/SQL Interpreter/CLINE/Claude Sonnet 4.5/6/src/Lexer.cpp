#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;
        
        tokens.push_back(scanToken());
    }
    
    tokens.push_back(Token(TokenType::END_OF_FILE));
    return tokens;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(peek())) {
        advance();
    }
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return input_[pos_];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    return input_[pos_++];
}

bool Lexer::isAtEnd() const {
    return pos_ >= input_.length();
}

Token Lexer::scanToken() {
    char c = peek();
    
    // Single character tokens
    switch (c) {
        case '(':
            advance();
            return Token(TokenType::LPAREN, "(");
        case ')':
            advance();
            return Token(TokenType::RPAREN, ")");
        case ',':
            advance();
            return Token(TokenType::COMMA, ",");
        case ';':
            advance();
            return Token(TokenType::SEMICOLON, ";");
        case '*':
            advance();
            return Token(TokenType::ASTERISK, "*");
        case '=':
            advance();
            return Token(TokenType::EQUALS, "=");
        case '"':
        case '\'':
            return scanStringLiteral();
    }
    
    // Numbers
    if (std::isdigit(c)) {
        return scanNumber();
    }
    
    // Identifiers and keywords
    if (std::isalpha(c) || c == '_') {
        return scanIdentifierOrKeyword();
    }
    
    throw std::runtime_error("Unexpected character: " + std::string(1, c));
}

Token Lexer::scanIdentifierOrKeyword() {
    std::string value;
    
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        value += advance();
    }
    
    std::string upperValue = Utils::toUpper(value);
    
    if (isKeyword(upperValue)) {
        return Token(TokenType::KEYWORD, upperValue);
    }
    
    return Token(TokenType::IDENTIFIER, value);
}

Token Lexer::scanStringLiteral() {
    char quote = advance(); // consume opening quote
    std::string value;
    
    while (!isAtEnd() && peek() != quote) {
        value += advance();
    }
    
    if (isAtEnd()) {
        throw std::runtime_error("Unterminated string literal");
    }
    
    advance(); // consume closing quote
    return Token(TokenType::STRING_LITERAL, value);
}

Token Lexer::scanNumber() {
    std::string value;
    
    while (!isAtEnd() && std::isdigit(peek())) {
        value += advance();
    }
    
    // Handle decimal point
    if (!isAtEnd() && peek() == '.') {
        value += advance();
        while (!isAtEnd() && std::isdigit(peek())) {
            value += advance();
        }
    }
    
    return Token(TokenType::NUMBER, value);
}

bool Lexer::isKeyword(const std::string& word) const {
    return word == "CREATE" || word == "TABLE" || 
           word == "INSERT" || word == "INTO" || 
           word == "VALUES" || word == "SELECT" || 
           word == "FROM" || word == "WHERE";
}
