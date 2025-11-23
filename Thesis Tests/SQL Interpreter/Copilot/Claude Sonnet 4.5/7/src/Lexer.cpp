#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (pos_ < input_.size()) {
        skipWhitespace();
        
        if (pos_ >= input_.size()) {
            break;
        }
        
        char c = currentChar();
        
        // Handle symbols
        if (c == '(' || c == ')' || c == ',' || c == ';' || c == '*' || c == '=') {
            tokens.push_back(Token(TokenType::SYMBOL, std::string(1, c)));
            advance();
        }
        // Handle string literals
        else if (c == '"' || c == '\'') {
            tokens.push_back(readString());
        }
        // Handle numbers
        else if (std::isdigit(c)) {
            tokens.push_back(readNumber());
        }
        // Handle identifiers and keywords
        else if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        }
        else {
            throw std::runtime_error("Unexpected character: " + std::string(1, c));
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_INPUT, ""));
    return tokens;
}

char Lexer::currentChar() {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_];
}

char Lexer::peek(size_t offset) {
    if (pos_ + offset >= input_.size()) {
        return '\0';
    }
    return input_[pos_ + offset];
}

void Lexer::advance() {
    if (pos_ < input_.size()) {
        pos_++;
    }
}

void Lexer::skipWhitespace() {
    while (pos_ < input_.size() && std::isspace(currentChar())) {
        advance();
    }
}

Token Lexer::readIdentifierOrKeyword() {
    std::string word;
    
    while (pos_ < input_.size() && (std::isalnum(currentChar()) || currentChar() == '_')) {
        word += currentChar();
        advance();
    }
    
    std::string upperWord = Utils::toUpper(word);
    if (isKeyword(upperWord)) {
        return Token(TokenType::KEYWORD, upperWord);
    }
    
    return Token(TokenType::IDENTIFIER, word);
}

Token Lexer::readString() {
    char quote = currentChar();
    advance(); // Skip opening quote
    
    std::string value;
    
    while (pos_ < input_.size() && currentChar() != quote) {
        if (currentChar() == '\\' && peek() == quote) {
            advance(); // Skip backslash
            value += currentChar();
            advance();
        } else {
            value += currentChar();
            advance();
        }
    }
    
    if (currentChar() != quote) {
        throw std::runtime_error("Unterminated string literal");
    }
    
    advance(); // Skip closing quote
    return Token(TokenType::STRING_LITERAL, value);
}

Token Lexer::readNumber() {
    std::string number;
    
    while (pos_ < input_.size() && (std::isdigit(currentChar()) || currentChar() == '.')) {
        number += currentChar();
        advance();
    }
    
    return Token(TokenType::NUMBER, number);
}

bool Lexer::isKeyword(const std::string& word) {
    static const std::vector<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", 
        "SELECT", "FROM", "WHERE"
    };
    
    for (const auto& keyword : keywords) {
        if (word == keyword) {
            return true;
        }
    }
    
    return false;
}
