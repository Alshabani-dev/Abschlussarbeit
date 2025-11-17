#include "Lexer.h"
#include "Utils.h"
#include <cctype>

Lexer::Lexer(const std::string& input)
    : input_(input), position_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (position_ < input_.length()) {
        skipWhitespace();
        
        if (position_ >= input_.length()) {
            break;
        }
        
        char current = peek();
        
        // Single character tokens
        if (current == '(') {
            tokens.push_back(Token(TokenType::LEFT_PAREN, "(", position_));
            advance();
        } else if (current == ')') {
            tokens.push_back(Token(TokenType::RIGHT_PAREN, ")", position_));
            advance();
        } else if (current == ',') {
            tokens.push_back(Token(TokenType::COMMA, ",", position_));
            advance();
        } else if (current == ';') {
            tokens.push_back(Token(TokenType::SEMICOLON, ";", position_));
            advance();
        } else if (current == '*') {
            tokens.push_back(Token(TokenType::ASTERISK, "*", position_));
            advance();
        } else if (current == '=') {
            tokens.push_back(Token(TokenType::EQUALS, "=", position_));
            advance();
        }
        // String literal
        else if (current == '"') {
            tokens.push_back(readStringLiteral());
        }
        // Number
        else if (std::isdigit(current)) {
            tokens.push_back(readNumber());
        }
        // Identifier or keyword
        else if (std::isalpha(current) || current == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        }
        else {
            error_ = "Unexpected character: " + std::string(1, current);
            tokens.push_back(Token(TokenType::UNKNOWN, std::string(1, current), position_));
            advance();
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_FILE, "", position_));
    return tokens;
}

char Lexer::peek() const {
    if (position_ >= input_.length()) {
        return '\0';
    }
    return input_[position_];
}

char Lexer::advance() {
    if (position_ >= input_.length()) {
        return '\0';
    }
    return input_[position_++];
}

void Lexer::skipWhitespace() {
    while (position_ < input_.length() && std::isspace(peek())) {
        advance();
    }
}

Token Lexer::readIdentifierOrKeyword() {
    size_t start = position_;
    std::string value;
    
    while (position_ < input_.length() && 
           (std::isalnum(peek()) || peek() == '_')) {
        value += advance();
    }
    
    TokenType type;
    if (isKeyword(value, type)) {
        return Token(type, value, start);
    }
    
    return Token(TokenType::IDENTIFIER, value, start);
}

Token Lexer::readStringLiteral() {
    size_t start = position_;
    advance(); // Skip opening quote
    
    std::string value;
    while (position_ < input_.length() && peek() != '"') {
        if (peek() == '\\' && position_ + 1 < input_.length()) {
            advance(); // Skip backslash
            char escaped = advance();
            // Handle basic escapes
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default: value += escaped; break;
            }
        } else {
            value += advance();
        }
    }
    
    if (peek() == '"') {
        advance(); // Skip closing quote
    } else {
        error_ = "Unterminated string literal";
    }
    
    return Token(TokenType::STRING_LITERAL, value, start);
}

Token Lexer::readNumber() {
    size_t start = position_;
    std::string value;
    
    while (position_ < input_.length() && std::isdigit(peek())) {
        value += advance();
    }
    
    // Handle decimal point
    if (peek() == '.' && position_ + 1 < input_.length() && 
        std::isdigit(input_[position_ + 1])) {
        value += advance(); // Add decimal point
        while (position_ < input_.length() && std::isdigit(peek())) {
            value += advance();
        }
    }
    
    return Token(TokenType::NUMBER, value, start);
}

bool Lexer::isKeyword(const std::string& str, TokenType& outType) {
    std::string upper = Utils::toUpper(str);
    
    if (upper == "CREATE") {
        outType = TokenType::CREATE;
        return true;
    } else if (upper == "TABLE") {
        outType = TokenType::TABLE;
        return true;
    } else if (upper == "INSERT") {
        outType = TokenType::INSERT;
        return true;
    } else if (upper == "INTO") {
        outType = TokenType::INTO;
        return true;
    } else if (upper == "VALUES") {
        outType = TokenType::VALUES;
        return true;
    } else if (upper == "SELECT") {
        outType = TokenType::SELECT;
        return true;
    } else if (upper == "FROM") {
        outType = TokenType::FROM;
        return true;
    } else if (upper == "WHERE") {
        outType = TokenType::WHERE;
        return true;
    }
    
    return false;
}
