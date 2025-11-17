#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,        // CREATE, TABLE, INSERT, INTO, VALUES, SELECT, FROM, WHERE
    IDENTIFIER,     // table/column names
    STRING_LITERAL, // "quoted string"
    NUMBER,         // numeric literal
    SYMBOL,         // ( ) , ; * =
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    
    Token(TokenType t, const std::string& v) : type(t), value(v) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    
    std::vector<Token> tokenize();
    
private:
    std::string input_;
    size_t pos_;
    
    char peek() const;
    char advance();
    void skipWhitespace();
    
    bool isKeyword(const std::string& word) const;
    Token readIdentifierOrKeyword();
    Token readStringLiteral();
    Token readNumber();
};

#endif // LEXER_H
