#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    explicit Parser(const std::string &input);

    Statement parseStatement();

private:
    Lexer lexer_;

    Token expect(TokenType type, const std::string &message);
    bool match(TokenType type);

    CreateTableStatement parseCreateTable();
    InsertStatement parseInsert();
    SelectStatement parseSelect();
};

#endif // PARSER_H
