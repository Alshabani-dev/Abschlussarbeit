#pragma once

#include <vector>
#include <memory>
#include "Lexer.h"
#include "Ast.h"

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Statement> parse();

private:
    std::vector<Token> tokens_;
    size_t position_;

    Token peek() const;
    Token consume();
    std::unique_ptr<Statement> parseCreateTable();
    std::unique_ptr<Statement> parseInsert();
    std::unique_ptr<Statement> parseSelect();
};
