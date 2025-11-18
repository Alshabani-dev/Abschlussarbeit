#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    END,
    IDENTIFIER,
    NUMBER,
    STRING,
    COMMA,
    LPAREN,
    RPAREN,
    STAR,
    SEMICOLON,
    EQUALS,
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
    size_t position;
};

class Lexer {
public:
    explicit Lexer(const std::string &input);
    Token nextToken();

private:
    std::string input_;
    size_t pos_ = 0;

    void skipWhitespace();
    char peek() const;
    char get();
    Token identifier();
    Token number();
    Token stringLiteral();
};

const char *tokenTypeToString(TokenType type);

#endif // LEXER_H
