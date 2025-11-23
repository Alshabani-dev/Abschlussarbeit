#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    CREATE,
    TABLE,
    INSERT,
    INTO,
    VALUES,
    SELECT,
    FROM,
    WHERE,
    
    // Symbols
    LEFT_PAREN,    // (
    RIGHT_PAREN,   // )
    COMMA,         // ,
    SEMICOLON,     // ;
    ASTERISK,      // *
    EQUALS,        // =
    
    // Literals and identifiers
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
    size_t position;
    
    Token(TokenType t, const std::string& v, size_t pos)
        : type(t), value(v), position(pos) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    
    std::vector<Token> tokenize();
    std::string getError() const { return error_; }
    
private:
    std::string input_;
    size_t position_;
    std::string error_;
    
    char peek() const;
    char advance();
    void skipWhitespace();
    
    Token readIdentifierOrKeyword();
    Token readStringLiteral();
    Token readNumber();
    
    bool isKeyword(const std::string& str, TokenType& outType);
};

#endif // LEXER_H
