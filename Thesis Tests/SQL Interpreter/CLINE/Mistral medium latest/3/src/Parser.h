#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include "Lexer.h"
#include "Ast.h"

class Parser {
public:
    Parser();
    std::unique_ptr<Statement> parse(const std::string &sql);

private:
    Lexer lexer_;
    std::vector<Token> tokens_;
    size_t currentToken_;

    void advance();
    Token peek() const;
    Token consume();

    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
};

#endif // PARSER_H
