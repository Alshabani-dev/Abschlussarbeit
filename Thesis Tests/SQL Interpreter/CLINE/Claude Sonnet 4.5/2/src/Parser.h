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
    std::string getLastError() const;
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    std::string lastError_;
    
    const Token& peek() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool checkValue(const std::string& value) const;
    bool match(TokenType type);
    bool matchValue(const std::string& value);
    bool expect(TokenType type, const std::string& errorMsg);
    bool expectValue(const std::string& value, const std::string& errorMsg);
    
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
};

#endif // PARSER_H
