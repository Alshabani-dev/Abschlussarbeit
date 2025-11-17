#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

struct Token {
    enum class Type {
        KEYWORD,
        IDENTIFIER,
        STRING,
        NUMBER,
        SYMBOL,
        END
    };

    Type type;
    std::string value;
};

class Lexer {
public:
    Lexer(const std::string& input);
    std::vector<Token> tokenize();

private:
    std::string input_;
    size_t position_ = 0;
};

#endif // LEXER_H
