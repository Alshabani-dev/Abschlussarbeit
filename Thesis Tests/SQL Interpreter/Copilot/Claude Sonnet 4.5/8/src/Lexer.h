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
    
    char currentChar() const;
    char peek(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    
    Token readIdentifierOrKeyword();
    Token readStringLiteral();
    Token readNumber();
    
    bool isKeyword(const std::string& word) const;
    TokenType getKeywordType(const std::string& word) const;
};

#endif // LEXER_H
