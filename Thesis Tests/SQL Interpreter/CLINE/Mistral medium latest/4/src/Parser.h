#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <memory>
#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    Parser();
    std::unique_ptr<Statement> parse(const std::string &sql);

private:
    Lexer lexer_;
    std::vector<Token> tokens_;
    size_t currentToken_;

    void advance();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
};

#endif // PARSER_H
