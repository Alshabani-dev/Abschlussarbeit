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
    size_t line;
    size_t column;
};

class Lexer {
public:
    Lexer(const std::string& input);
    std::vector<Token> tokenize();

private:
    std::string input_;
    size_t position_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;

    char peek() const;
    char consume();
    void skipWhitespace();
    Token readIdentifierOrKeyword();
    Token readStringLiteral();
    Token readNumber();
    Token readSymbol();
};

#endif // LEXER_H
