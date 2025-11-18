#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    STRING_LITERAL,
    NUMBER,
    LPAREN,      // (
    RPAREN,      // )
    COMMA,       // ,
    SEMICOLON,   // ;
    ASTERISK,    // *
    EQUALS,      // =
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    
    Token(TokenType t, const std::string& v = "") : type(t), value(v) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    std::vector<Token> tokenize();

private:
    std::string input_;
    size_t pos_;
    
    void skipWhitespace();
    char peek() const;
    char advance();
    bool isAtEnd() const;
    
    Token scanToken();
    Token scanIdentifierOrKeyword();
    Token scanStringLiteral();
    Token scanNumber();
    
    bool isKeyword(const std::string& word) const;
};

#endif // LEXER_H
