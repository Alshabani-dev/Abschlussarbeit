#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "Ast.h"
#include <memory>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Statement> parse();

private:
    std::vector<Token> tokens_;
    size_t position_ = 0;

    Token peek() const;
    Token consume();
    void expect(TokenType type, const std::string& value = "");

    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
};

#endif // PARSER_H
