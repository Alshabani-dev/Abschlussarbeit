#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (pos_ < input_.length()) {
        skipWhitespace();
        
        if (pos_ >= input_.length()) {
            break;
        }
        
        char c = current();
        
        // Handle symbols
        if (c == '(' || c == ')' || c == ',' || c == ';' || c == '*' || c == '=') {
            tokens.push_back(Token(TokenType::SYMBOL, std::string(1, c)));
            advance();
        }
        // Handle string literals
        else if (c == '"' || c == '\'') {
            tokens.push_back(readStringLiteral());
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
            throw std::runtime_error(std::string("Unexpected character: ") + c);
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_INPUT, ""));
    return tokens;
}

char Lexer::current() const {
    if (pos_ < input_.length()) {
        return input_[pos_];
    }
    return '\0';
}

char Lexer::peek(size_t offset) const {
    if (pos_ + offset < input_.length()) {
        return input_[pos_ + offset];
    }
    return '\0';
}

void Lexer::advance() {
    if (pos_ < input_.length()) {
        pos_++;
    }
}

void Lexer::skipWhitespace() {
    while (pos_ < input_.length() && std::isspace(static_cast<unsigned char>(current()))) {
        advance();
    }
}

Token Lexer::readIdentifierOrKeyword() {
    std::string value;
    
    while (pos_ < input_.length() && (std::isalnum(static_cast<unsigned char>(current())) || current() == '_')) {
        value += current();
        advance();
    }
    
    // Check if it's a keyword
    if (isKeyword(value)) {
        return Token(TokenType::KEYWORD, Utils::toUpper(value));
    }
    
    // Normalize identifier to lowercase
    return Token(TokenType::IDENTIFIER, Utils::toLower(value));
}

Token Lexer::readStringLiteral() {
    char quote = current();
    advance(); // Skip opening quote
    
    std::string value;
    
    while (pos_ < input_.length() && current() != quote) {
        if (current() == '\\' && peek() == quote) {
            // Escaped quote
            advance();
            value += current();
            advance();
        } else {
            value += current();
            advance();
        }
    }
    
    if (current() != quote) {
        throw std::runtime_error("Unterminated string literal");
    }
    
    advance(); // Skip closing quote
    
    return Token(TokenType::STRING_LITERAL, value);
}

Token Lexer::readNumber() {
    std::string value;
    
    while (pos_ < input_.length() && (std::isdigit(static_cast<unsigned char>(current())) || current() == '.')) {
        value += current();
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
