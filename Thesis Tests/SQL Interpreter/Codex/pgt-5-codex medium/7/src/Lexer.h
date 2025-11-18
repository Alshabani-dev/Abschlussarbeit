#ifndef LEXER_H
#define LEXER_H

#include <cstddef>
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
    KeywordCreate,
    KeywordTable,
    KeywordInsert,
    KeywordInto,
    KeywordValues,
    KeywordSelect,
    KeywordFrom,
    KeywordWhere,
    End
};

struct Token {
    TokenType type;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(const std::string &input);

    Token peek();
    Token next();

private:
    std::string input_;
    size_t position_;
    Token current_;
    bool hasCurrent_;

    void skipWhitespace();
    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readString();
};

#endif // LEXER_H
