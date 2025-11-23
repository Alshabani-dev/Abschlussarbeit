#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include "Lexer.h"
#include "Ast.h"

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::unique_ptr<Statement> parseStatement();
    std::string getError() const { return error_; }
    
private:
    std::vector<Token> tokens_;
    size_t current_;
    std::string error_;
    
    const Token& peek() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool expect(TokenType type, const std::string& errorMsg);
    
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
    
    bool isAtEnd() const;
};

#endif // PARSER_H
