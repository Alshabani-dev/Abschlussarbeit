#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    END,
    IDENTIFIER,
    STRING_LITERAL,
    STAR,
    COMMA,
    LPAREN,
    RPAREN,
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
};

class Lexer {
public:
    explicit Lexer(const std::string &input);
    std::vector<Token> tokenize();

private:
    std::string input_;
    std::size_t position_;

    bool atEnd() const;
    char peek() const;
    char advance();
    void skipWhitespace();
    Token parseIdentifier();
    Token parseStringLiteral();
};

#endif // LEXER_H
