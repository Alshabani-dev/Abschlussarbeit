#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    STRING,
    NUMBER,
    SYMBOL,
    SEMICOLON,
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    
    Token(TokenType t, const std::string& v) : type(t), value(v) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    
    std::vector<Token> tokenize();
    
private:
    std::string input_;
    size_t pos_;
    
    void skipWhitespace();
    bool isAtEnd() const;
    char peek() const;
    char advance();
    bool match(char expected);
    
    Token readString();
    Token readNumber();
    Token readIdentifierOrKeyword();
    
    bool isKeyword(const std::string& word) const;
};

#endif // LEXER_H
