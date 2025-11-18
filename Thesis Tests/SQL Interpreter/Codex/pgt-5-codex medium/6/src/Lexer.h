#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    END,
    IDENTIFIER,
    NUMBER,
    STRING,
    STAR,
    COMMA,
    SEMICOLON,
    LPAREN,
    RPAREN,
    EQUAL,
    KEYWORD_CREATE,
    KEYWORD_TABLE,
    KEYWORD_INSERT,
    KEYWORD_INTO,
    KEYWORD_VALUES,
    KEYWORD_SELECT,
    KEYWORD_FROM,
    KEYWORD_WHERE
};

struct Token {
    TokenType type;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(const std::string &source);
    std::vector<Token> tokenize();

private:
    char peek() const;
    char get();
    void skipWhitespace();
    Token identifierOrKeyword();
    Token number();
    Token stringLiteral();

    std::string source_;
    size_t position_ = 0;
};

#endif // LEXER_H
