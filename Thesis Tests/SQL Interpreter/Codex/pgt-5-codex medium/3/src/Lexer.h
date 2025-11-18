#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    Identifier,
    Number,
    StringLiteral,
    Comma,
    LParen,
    RParen,
    Semicolon,
    Star,
    Equals,
    End
};

struct Token {
    TokenType type;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(const std::string &input);

    const Token &peek(size_t offset = 0) const;
    Token consume();

private:
    void tokenize();

    std::string input_;
    std::vector<Token> tokens_;
    size_t position_ = 0;
};

#endif // LEXER_H
