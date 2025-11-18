#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    END,
    IDENTIFIER,
    STRING_LITERAL,
    COMMA,
    LPAREN,
    RPAREN,
    STAR,
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
    explicit Lexer(const std::string &input);
    std::vector<Token> tokenize();
    std::string getError() const;

private:
    std::string input_;
    size_t position_;
    std::string error_;

    void skipWhitespace();
    bool eof() const;
    char peek() const;
    char advance();
    Token identifier();
    Token stringLiteral();
};

#endif // LEXER_H
