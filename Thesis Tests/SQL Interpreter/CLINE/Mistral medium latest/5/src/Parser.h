#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <memory>
#include "Lexer.h"
#include "Ast.h"

class Parser {
public:
    Parser(const std::vector<Token> &tokens);
    std::unique_ptr<Statement> parse();

private:
    std::vector<Token> tokens_;
    size_t position_ = 0;

    Token peek() const;
    Token consume();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
    std::vector<std::string> parseColumnList();
    std::vector<std::string> parseValueList();
    std::pair<std::string, std::string> parseWhereClause();
};

#endif // PARSER_H
