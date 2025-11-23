#include "Lexer.h"
#include "Utils.h"
#include <cctype>

Lexer::Lexer(const std::string& input) : input_(input), pos_(0) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (pos_ < input_.length()) {
        skipWhitespace();
        
        if (pos_ >= input_.length()) {
            break;
        }
        
        char ch = peek();
        
        // Single character tokens
        if (ch == '(') {
            tokens.push_back(Token(TokenType::LPAREN, "("));
            advance();
        } else if (ch == ')') {
            tokens.push_back(Token(TokenType::RPAREN, ")"));
            advance();
        } else if (ch == ',') {
            tokens.push_back(Token(TokenType::COMMA, ","));
            advance();
        } else if (ch == ';') {
            tokens.push_back(Token(TokenType::SEMICOLON, ";"));
            advance();
        } else if (ch == '*') {
            tokens.push_back(Token(TokenType::ASTERISK, "*"));
            advance();
        } else if (ch == '=') {
            tokens.push_back(Token(TokenType::EQUALS, "="));
            advance();
        } else if (ch == '"' || ch == '\'') {
            tokens.push_back(readString());
        } else if (std::isdigit(ch) || (ch == '-' && pos_ + 1 < input_.length() && std::isdigit(input_[pos_ + 1]))) {
            tokens.push_back(readNumber());
        } else if (std::isalpha(ch) || ch == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else {
            // Invalid character, skip it
            advance();
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_FILE, ""));
    return tokens;
}

char Lexer::peek() const {
    if (pos_ < input_.length()) {
        return input_[pos_];
    }
    return '\0';
}

char Lexer::advance() {
    if (pos_ < input_.length()) {
        return input_[pos_++];
    }
    return '\0';
}

void Lexer::skipWhitespace() {
    while (pos_ < input_.length() && std::isspace(input_[pos_])) {
        ++pos_;
    }
}

Token Lexer::readIdentifierOrKeyword() {
    std::string value;
    
    while (pos_ < input_.length() && (std::isalnum(input_[pos_]) || input_[pos_] == '_')) {
        value += input_[pos_];
        ++pos_;
    }
    
    std::string upperValue = Utils::toUpper(value);
    
    if (isKeyword(upperValue)) {
        return Token(getKeywordType(upperValue), value);
    }
    
    return Token(TokenType::IDENTIFIER, value);
}

Token Lexer::readString() {
    char quote = advance(); // consume opening quote
    std::string value;
    
    while (pos_ < input_.length() && input_[pos_] != quote) {
        if (input_[pos_] == '\\' && pos_ + 1 < input_.length()) {
            // Handle escape sequences
            ++pos_;
            char escaped = input_[pos_];
            if (escaped == 'n') {
                value += '\n';
            } else if (escaped == 't') {
                value += '\t';
            } else if (escaped == '\\') {
                value += '\\';
            } else if (escaped == quote) {
                value += quote;
            } else {
                value += escaped;
            }
            ++pos_;
        } else {
            value += input_[pos_];
            ++pos_;
        }
    }
    
    if (pos_ < input_.length()) {
        ++pos_; // consume closing quote
    }
    
    return Token(TokenType::STRING, value);
}

Token Lexer::readNumber() {
    std::string value;
    
    if (peek() == '-') {
        value += advance();
    }
    
    while (pos_ < input_.length() && (std::isdigit(input_[pos_]) || input_[pos_] == '.')) {
        value += input_[pos_];
        ++pos_;
    }
    
    return Token(TokenType::NUMBER, value);
}

bool Lexer::isKeyword(const std::string& word) const {
    return word == "CREATE" || word == "TABLE" || word == "INSERT" ||
           word == "INTO" || word == "VALUES" || word == "SELECT" ||
           word == "FROM" || word == "WHERE";
}

TokenType Lexer::getKeywordType(const std::string& word) const {
    if (word == "CREATE") return TokenType::CREATE;
    if (word == "TABLE") return TokenType::TABLE;
    if (word == "INSERT") return TokenType::INSERT;
    if (word == "INTO") return TokenType::INTO;
    if (word == "VALUES") return TokenType::VALUES;
    if (word == "SELECT") return TokenType::SELECT;
    if (word == "FROM") return TokenType::FROM;
    if (word == "WHERE") return TokenType::WHERE;
    
    return TokenType::INVALID;
}
