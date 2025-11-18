#include "Lexer.h"
#include "Utils.h"
#include <cctype>

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (pos_ < input_.size()) {
        skipWhitespace();
        
        if (pos_ >= input_.size()) {
            break;
        }
        
        char c = currentChar();
        
        // Single character tokens
        if (c == '(') {
            tokens.emplace_back(TokenType::LPAREN, "(");
            advance();
        } else if (c == ')') {
            tokens.emplace_back(TokenType::RPAREN, ")");
            advance();
        } else if (c == ',') {
            tokens.emplace_back(TokenType::COMMA, ",");
            advance();
        } else if (c == ';') {
            tokens.emplace_back(TokenType::SEMICOLON, ";");
            advance();
        } else if (c == '*') {
            tokens.emplace_back(TokenType::ASTERISK, "*");
            advance();
        } else if (c == '=') {
            tokens.emplace_back(TokenType::EQUALS, "=");
            advance();
        } else if (c == '"' || c == '\'') {
            tokens.push_back(readStringLiteral());
        } else if (std::isdigit(c)) {
            tokens.push_back(readNumber());
        } else if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else {
            tokens.emplace_back(TokenType::UNKNOWN, std::string(1, c));
            advance();
        }
    }
    
    tokens.emplace_back(TokenType::END_OF_FILE, "");
    return tokens;
}

char Lexer::currentChar() const {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_];
}

char Lexer::peek(size_t offset) const {
    size_t peekPos = pos_ + offset;
    if (peekPos >= input_.size()) {
        return '\0';
    }
    return input_[peekPos];
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
    std::string value;
    
    while (pos_ < input_.size() && (std::isalnum(currentChar()) || currentChar() == '_')) {
        value += currentChar();
        advance();
    }
    
    if (isKeyword(value)) {
        return Token(getKeywordType(value), value);
    }
    
    return Token(TokenType::IDENTIFIER, value);
}

Token Lexer::readStringLiteral() {
    char quoteChar = currentChar();
    advance(); // Skip opening quote
    
    std::string value;
    
    while (pos_ < input_.size() && currentChar() != quoteChar) {
        if (currentChar() == '\\' && peek() == quoteChar) {
            advance(); // Skip backslash
            value += currentChar();
            advance();
        } else {
            value += currentChar();
            advance();
        }
    }
    
    if (pos_ < input_.size()) {
        advance(); // Skip closing quote
    }
    
    return Token(TokenType::STRING_LITERAL, value);
}

Token Lexer::readNumber() {
    std::string value;
    
    while (pos_ < input_.size() && (std::isdigit(currentChar()) || currentChar() == '.')) {
        value += currentChar();
        advance();
    }
    
    return Token(TokenType::NUMBER, value);
}

bool Lexer::isKeyword(const std::string& word) const {
    std::string upper = Utils::toUpper(word);
    return upper == "CREATE" || upper == "TABLE" || upper == "INSERT" ||
           upper == "INTO" || upper == "VALUES" || upper == "SELECT" ||
           upper == "FROM" || upper == "WHERE";
}

TokenType Lexer::getKeywordType(const std::string& word) const {
    std::string upper = Utils::toUpper(word);
    
    if (upper == "CREATE") return TokenType::CREATE;
    if (upper == "TABLE") return TokenType::TABLE;
    if (upper == "INSERT") return TokenType::INSERT;
    if (upper == "INTO") return TokenType::INTO;
    if (upper == "VALUES") return TokenType::VALUES;
    if (upper == "SELECT") return TokenType::SELECT;
    if (upper == "FROM") return TokenType::FROM;
    if (upper == "WHERE") return TokenType::WHERE;
    
    return TokenType::UNKNOWN;
}
