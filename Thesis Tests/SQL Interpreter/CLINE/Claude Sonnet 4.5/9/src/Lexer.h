#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    SYMBOL,
    STRING_LITERAL,
    NUMBER,
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
    
    char current() const;
    char peek(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    
    Token readIdentifierOrKeyword();
    Token readStringLiteral();
    Token readNumber();
    
    bool isKeyword(const std::string& word) const;
};

#endif // LEXER_H
