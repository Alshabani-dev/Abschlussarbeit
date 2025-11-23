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
    
    Token peek() const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool matchKeyword(const std::string& keyword);
    
    Token expect(TokenType type, const std::string& errorMessage);
    Token expectKeyword(const std::string& keyword, const std::string& errorMessage);
    
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
};

#endif // PARSER_H
