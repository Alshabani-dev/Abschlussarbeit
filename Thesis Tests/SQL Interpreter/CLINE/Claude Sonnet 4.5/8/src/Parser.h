#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "Ast.h"
#include <memory>
#include <stdexcept>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::unique_ptr<Statement> parseStatement();
    
    std::string getError() const { return error_; }
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    std::string error_;
    
    const Token& currentToken() const;
    const Token& peek(size_t offset = 1) const;
    void advance();
    bool match(TokenType type);
    bool expect(TokenType type, const std::string& errorMsg);
    
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
};

#endif // PARSER_H
