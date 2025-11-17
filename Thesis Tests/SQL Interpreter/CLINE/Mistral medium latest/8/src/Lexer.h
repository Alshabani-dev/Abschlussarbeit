#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    CREATE, TABLE, INSERT, INTO, VALUES, SELECT, FROM, WHERE,
    // Symbols
    LPAREN, RPAREN, COMMA, SEMICOLON, ASTERISK, EQUALS,
    // Identifiers and literals
    IDENTIFIER, STRING_LITERAL, NUMBER,
    // End of input
    END
};

struct Token {
    TokenType type;
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
