#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "Lexer.h"
#include "Ast.h"

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Statement> parse();

private:
    std::vector<Token> tokens_;
    size_t position_ = 0;

    std::unique_ptr<Statement> parseCreateTable();
    std::unique_ptr<Statement> parseInsert();
    std::unique_ptr<Statement> parseSelect();
};

#endif // PARSER_H
