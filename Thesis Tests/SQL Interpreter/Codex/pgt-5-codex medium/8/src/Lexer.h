#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <unordered_set>

enum class TokenType {
    Identifier,
    Number,
    String,
    Comma,
    LParen,
    RParen,
    Star,
    Equal,
    Semicolon,
    Keyword,
    End,
    Unknown
};

struct Token {
    TokenType type{TokenType::Unknown};
    std::string text;
};

class Lexer {
public:
    explicit Lexer(const std::string &input);
    Token next();
    Token peek();

private:
    const std::string input_;
    size_t pos_{0};
    Token lookahead_;
    bool hasLookahead_{false};

    char current() const;
    void advance();
    void skipWhitespace();
    Token makeKeywordOrIdentifier(const std::string &value) const;
    Token readIdentifier();
    Token readNumber();
    Token readString();
};

#endif // LEXER_H
