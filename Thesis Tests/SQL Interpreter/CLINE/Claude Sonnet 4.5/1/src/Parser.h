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
    
    std::string getError() const { return error_; }
    bool hasError() const { return !error_.empty(); }
    
private:
    std::vector<Token> tokens_;
    size_t current_;
    std::string error_;
    
    // Token navigation
    const Token& currentToken() const;
    const Token& peek(int offset = 1) const;
    bool isAtEnd() const;
    void advance();
    bool match(TokenType type);
    bool check(TokenType type) const;
    bool expect(TokenType type, const std::string& message);
    
    // Statement parsing
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
    
    // Helper methods
    std::vector<std::string> parseColumnList();
    std::vector<std::string> parseValueList();
};

#endif // PARSER_H
