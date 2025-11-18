#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "Ast.h"
#include <memory>
#include <vector>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::unique_ptr<Statement> parseStatement();
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    
    Token currentToken();
    Token peek(size_t offset = 1);
    void advance();
    
    bool expect(TokenType type, const std::string& value = "");
    void consume(TokenType type, const std::string& value = "");
    
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
    
    std::string parseIdentifier();
    std::vector<std::string> parseColumnList();
    std::vector<std::string> parseValueList();
};

#endif // PARSER_H
