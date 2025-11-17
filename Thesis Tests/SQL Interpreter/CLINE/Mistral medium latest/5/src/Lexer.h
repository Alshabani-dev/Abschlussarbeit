#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    STRING_LITERAL,
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
    Lexer(const std::string &input);
    std::vector<Token> tokenize();

private:
    std::string input_;
    size_t position_ = 0;

    char peek() const;
    char consume();
    void skipWhitespace();
    Token readNextToken();
    Token readIdentifierOrKeyword();
    Token readStringLiteral();
    Token readNumber();
    Token readSymbol();
};

#endif // LEXER_H
