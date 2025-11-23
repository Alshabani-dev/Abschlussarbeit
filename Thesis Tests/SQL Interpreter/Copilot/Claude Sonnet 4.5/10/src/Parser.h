#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "Ast.h"
#include <memory>
#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::unique_ptr<Statement> parse();
    std::string getError() const { return error_; }
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    std::string error_;
    
    bool isAtEnd() const;
    const Token& peek() const;
    const Token& advance();
    bool match(TokenType type);
    bool match(TokenType type, const std::string& value);
    
    void setError(const std::string& message);
    
    std::unique_ptr<Statement> parseCreateTable();
    std::unique_ptr<Statement> parseInsert();
    std::unique_ptr<Statement> parseSelect();
    
    bool expect(TokenType type, const std::string& message);
    bool expectSymbol(const std::string& symbol, const std::string& message);
    bool expectKeyword(const std::string& keyword, const std::string& message);
};

#endif // PARSER_H
