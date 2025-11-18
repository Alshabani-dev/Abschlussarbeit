#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    Identifier,
    Number,
    String,
    Comma,
    LParen,
    RParen,
    Semicolon,
    Star,
    Equal,
    Keyword,
    EndOfFile
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
    size_t position_ = 0;

    void skipWhitespace();
    char peek() const;
    char get();
    bool eof() const;
    Token identifier();
    Token number();
    Token stringLiteral();
};

#endif // LEXER_H
