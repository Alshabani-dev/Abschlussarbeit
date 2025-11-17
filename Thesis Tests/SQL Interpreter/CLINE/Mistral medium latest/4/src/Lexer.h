#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    STRING,
    NUMBER,
    SYMBOL,
    END_OF_INPUT
};

struct Token {
    TokenType type;
    std::string value;
    size_t position;
};

class Lexer {
public:
    Lexer();
    std::vector<Token> tokenize(const std::string &input);

private:
    std::vector<Token> tokens_;
    size_t currentPos_;
    std::string input_;

    void skipWhitespace();
    Token nextToken();
    Token readIdentifierOrKeyword();
    Token readString();
    Token readNumber();
    Token readSymbol();
};

#endif // LEXER_H
