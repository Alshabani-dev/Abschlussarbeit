#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "Ast.h"
#include <memory>
#include <stdexcept>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    
    std::unique_ptr<Statement> parse();
    
private:
    std::vector<Token> tokens_;
    size_t pos_;
    
    const Token& current() const;
    const Token& peek(size_t offset = 1) const;
    void advance();
    bool match(TokenType type, const std::string& value = "");
    void expect(TokenType type, const std::string& value = "");
    
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
};

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message) 
        : std::runtime_error(message) {}
};

#endif // PARSER_H
