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
    std::vector<Token> tokenize(const std::string &input);
};

#endif // LEXER_H
