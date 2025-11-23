#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <unordered_set>

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

char Lexer::peek() const {
    if (pos_ >= input_.length()) {
        return '\0';
    }
    return input_[pos_];
}

char Lexer::advance() {
    if (pos_ >= input_.length()) {
        return '\0';
    }
    return input_[pos_++];
}

void Lexer::skipWhitespace() {
    while (pos_ < input_.length() && std::isspace(peek())) {
        advance();
    }
}

bool Lexer::isKeyword(const std::string& word) const {
    static const std::unordered_set<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", 
        "SELECT", "FROM", "WHERE"
    };
    
    std::string upper = Utils::toUpper(word);
    return keywords.find(upper) != keywords.end();
}

Token Lexer::readIdentifierOrKeyword() {
    std::string value;
    
    while (pos_ < input_.length() && 
           (std::isalnum(peek()) || peek() == '_')) {
        value += advance();
    }
    
    if (isKeyword(value)) {
        return Token(TokenType::KEYWORD, Utils::toUpper(value));
    }
    
    return Token(TokenType::IDENTIFIER, value);
}

Token Lexer::readStringLiteral() {
    std::string value;
    advance(); // Skip opening quote
    
    while (pos_ < input_.length() && peek() != '"') {
        if (peek() == '\\' && pos_ + 1 < input_.length()) {
            advance(); // Skip backslash
            value += advance(); // Add escaped character
        } else {
            value += advance();
        }
    }
    
    if (peek() == '"') {
        advance(); // Skip closing quote
    }
    
    return Token(TokenType::STRING_LITERAL, value);
}

Token Lexer::readNumber() {
    std::string value;
    
    while (pos_ < input_.length() && 
           (std::isdigit(peek()) || peek() == '.')) {
        value += advance();
    }
    
    return Token(TokenType::NUMBER, value);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (pos_ < input_.length()) {
        skipWhitespace();
        
        if (pos_ >= input_.length()) {
            break;
        }
        
        char c = peek();
        
        // Single character symbols
        if (c == '(' || c == ')' || c == ',' || c == ';' || 
            c == '*' || c == '=') {
            tokens.push_back(Token(TokenType::SYMBOL, std::string(1, c)));
            advance();
        }
        // String literal
        else if (c == '"') {
            tokens.push_back(readStringLiteral());
        }
        // Number
        else if (std::isdigit(c)) {
            tokens.push_back(readNumber());
        }
        // Identifier or keyword
        else if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        }
        // Unknown character
        else {
            tokens.push_back(Token(TokenType::UNKNOWN, std::string(1, c)));
            advance();
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_FILE, ""));
    return tokens;
}
