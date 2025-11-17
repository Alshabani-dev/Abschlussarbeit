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
    INVALID
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t, const std::string& v = "", int l = 1, int c = 1)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    
    std::vector<Token> tokenize();
    
    std::string getError() const { return error_; }
    
private:
    std::string input_;
    size_t position_;
    int line_;
    int column_;
    std::string error_;
    
    char current() const;
    char peek(int offset = 1) const;
    void advance();
    void skipWhitespace();
    
    Token readIdentifierOrKeyword();
    Token readStringLiteral();
    Token readNumber();
    
    bool isAlpha(char c) const;
    bool isDigit(char c) const;
    bool isAlphaNumeric(char c) const;
    
    TokenType identifierType(const std::string& text) const;
};

#endif // LEXER_H
