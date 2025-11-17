#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    CREATE, TABLE, INSERT, INTO, VALUES, SELECT, FROM, WHERE,
    
    // Symbols
    LPAREN,      // (
    RPAREN,      // )
    COMMA,       // ,
    SEMICOLON,   // ;
    ASTERISK,    // *
    EQUALS,      // =
    
    // Values
    IDENTIFIER,
    STRING_LITERAL,
    NUMBER,
    
    // Special
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    
    Token(TokenType t, const std::string& v = "") : type(t), value(v) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    std::vector<Token> tokenize();
    
private:
    std::string input_;
    size_t pos_;
    
    void skipWhitespace();
    char peek() const;
    char advance();
    bool isAtEnd() const;
    
    Token readIdentifierOrKeyword();
    Token readStringLiteral();
    Token readNumber();
    
    TokenType getKeywordType(const std::string& word) const;
};

#endif // LEXER_H
