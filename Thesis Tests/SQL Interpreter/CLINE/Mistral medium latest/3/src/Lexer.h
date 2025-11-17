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
    bool isKeyword(const std::string &value) const;
    std::vector<std::string> keywords_ = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES",
        "SELECT", "FROM", "WHERE", "AND", "OR"
    };
};

#endif // LEXER_H
