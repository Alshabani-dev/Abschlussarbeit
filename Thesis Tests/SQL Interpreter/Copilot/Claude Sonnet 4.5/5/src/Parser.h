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
    
    std::string getErrorMessage() const { return errorMessage_; }
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    std::string errorMessage_;
    
    const Token& peek() const;
    const Token& advance();
    bool match(TokenType type);
    bool expect(TokenType type, const std::string& errorMsg);
    
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
    
    void setError(const std::string& msg);
};

#endif // PARSER_H
