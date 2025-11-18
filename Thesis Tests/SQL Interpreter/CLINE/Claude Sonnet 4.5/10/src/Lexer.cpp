#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <set>

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        skipWhitespace();
        
        if (isAtEnd()) {
            break;
        }
        
        char c = peek();
        
        // Handle string literals
        if (c == '"' || c == '\'') {
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
        // Handle symbols
        else if (c == '(' || c == ')' || c == ',' || c == '*' || c == '=') {
            advance();
            tokens.push_back(Token(TokenType::SYMBOL, std::string(1, c)));
        }
        // Handle semicolon
        else if (c == ';') {
            advance();
            tokens.push_back(Token(TokenType::SEMICOLON, ";"));
        }
        else {
            // Skip unknown characters
            advance();
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_FILE, ""));
    return tokens;
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && std::isspace(static_cast<unsigned char>(peek()))) {
        advance();
    }
}

bool Lexer::isAtEnd() const {
    return pos_ >= input_.length();
}

char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }
    return input_[pos_];
}

char Lexer::advance() {
    if (isAtEnd()) {
        return '\0';
    }
    return input_[pos_++];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || peek() != expected) {
        return false;
    }
    advance();
    return true;
}

Token Lexer::readString() {
    char quote = advance(); // consume opening quote
    std::string value;
    
    while (!isAtEnd() && peek() != quote) {
        value += advance();
    }
    
    if (!isAtEnd()) {
        advance(); // consume closing quote
    }
    
    return Token(TokenType::STRING, value);
}

Token Lexer::readNumber() {
    std::string value;
    
    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.')) {
        value += advance();
    }
    
    return Token(TokenType::NUMBER, value);
}

Token Lexer::readIdentifierOrKeyword() {
    std::string value;
    
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        value += advance();
    }
    
    // Normalize to uppercase for keyword checking
    std::string upper = Utils::toUpper(value);
    
    if (isKeyword(upper)) {
        return Token(TokenType::KEYWORD, upper);
    }
    
    // Store identifiers in lowercase for case-insensitive table/column names
    return Token(TokenType::IDENTIFIER, Utils::toLower(value));
}

bool Lexer::isKeyword(const std::string& word) const {
    static const std::set<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", 
        "SELECT", "FROM", "WHERE"
    };
    
    return keywords.find(word) != keywords.end();
}
