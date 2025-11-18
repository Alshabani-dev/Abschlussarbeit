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
    Equal,
    End,
    Create,
    TableKW,
    Insert,
    Into,
    Values,
    Select,
    From,
    Where,
    Unknown
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
    size_t pos_ = 0;

    char peek() const;
    char get();
    void skipWhitespace();
    Token identifier();
    Token number();
    Token stringLiteral();
    static TokenType keywordType(const std::string &value);
};

#endif // LEXER_H
