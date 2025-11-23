#include "Lexer.h"
#include "Utils.h"
#include <cctype>

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;
        
        char c = peek();
        
        // Single-character tokens
        if (c == '(') {
            advance();
            tokens.emplace_back(TokenType::LPAREN, "(");
        } else if (c == ')') {
            advance();
            tokens.emplace_back(TokenType::RPAREN, ")");
        } else if (c == ',') {
            advance();
            tokens.emplace_back(TokenType::COMMA, ",");
        } else if (c == ';') {
            advance();
            tokens.emplace_back(TokenType::SEMICOLON, ";");
        } else if (c == '*') {
            advance();
            tokens.emplace_back(TokenType::ASTERISK, "*");
        } else if (c == '=') {
            advance();
            tokens.emplace_back(TokenType::EQUALS, "=");
        }
        // String literals
        else if (c == '"' || c == '\'') {
            tokens.push_back(readStringLiteral());
        }
        // Numbers
        else if (std::isdigit(c)) {
            tokens.push_back(readNumber());
        }
        // Identifiers and keywords
        else if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        }
        else {
            // Unknown character, skip it
            advance();
            tokens.emplace_back(TokenType::UNKNOWN, std::string(1, c));
        }
    }
    
    tokens.emplace_back(TokenType::END_OF_FILE, "");
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

Token Lexer::readIdentifierOrKeyword() {
    std::string value;
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        value += advance();
    }
    
    TokenType type = getKeywordType(value);
    if (type != TokenType::IDENTIFIER) {
        return Token(type, value);
    }
    
    return Token(TokenType::IDENTIFIER, value);
}

Token Lexer::readStringLiteral() {
    char quote = advance(); // Consume opening quote
    std::string value;
    
    while (!isAtEnd() && peek() != quote) {
        value += advance();
    }
    
    if (!isAtEnd()) {
        advance(); // Consume closing quote
    }
    
    return Token(TokenType::STRING_LITERAL, value);
}

Token Lexer::readNumber() {
    std::string value;
    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.')) {
        value += advance();
    }
    return Token(TokenType::NUMBER, value);
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
    
    return TokenType::IDENTIFIER;
}
