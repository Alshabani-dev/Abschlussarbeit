#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    STRING_LITERAL,
    NUMBER,
    SYMBOL,
    END_OF_INPUT
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
    
    char currentChar();
    char peek(size_t offset = 1);
    void advance();
    void skipWhitespace();
    
    Token readIdentifierOrKeyword();
    Token readString();
    Token readNumber();
    
    bool isKeyword(const std::string& word);
};

#endif // LEXER_H
