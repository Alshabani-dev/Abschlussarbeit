#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <unordered_map>
#include <vector>

struct Token {
    enum class Type {
        EndOfFile,
        Identifier,
        Number,
        String,
        Comma,
        Semicolon,
        LParen,
        RParen,
        Asterisk,
        Equal,
        Create,
        Table,
        Insert,
        Into,
        Values,
        Select,
        From,
        Where
    };

    Type type;
    std::string text;
};

class Lexer {
public:
    explicit Lexer(std::string input);
    std::vector<Token> tokenize();

private:
    char peek() const;
    char advance();
    bool eof() const;

    void skipWhitespace();
    Token identifierOrKeyword();
    Token number();
    Token stringLiteral();

    std::string input_;
    std::size_t pos_ = 0;
};

#endif // LEXER_H
