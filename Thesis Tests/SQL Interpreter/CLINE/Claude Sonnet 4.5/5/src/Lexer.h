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
    LPAREN,      // (
    RPAREN,      // )
    COMMA,       // ,
    SEMICOLON,   // ;
    ASTERISK,    // *
    EQUALS,      // =
    
    // Literals
    IDENTIFIER,
    STRING,
    NUMBER,
    
    // Special
    END_OF_FILE,
    INVALID
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
    
    char peek() const;
    char advance();
    void skipWhitespace();
    
    Token readIdentifierOrKeyword();
    Token readString();
    Token readNumber();
    
    bool isKeyword(const std::string& word) const;
    TokenType getKeywordType(const std::string& word) const;
};

#endif // LEXER_H
