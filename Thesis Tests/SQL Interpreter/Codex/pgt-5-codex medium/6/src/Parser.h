#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <string>
#include <vector>

#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    explicit Parser(const std::string &source);
    std::vector<StatementPtr> parseStatements();

private:
    const Token &peek(size_t offset = 0) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string &message);
    Token consumeIdentifier(const std::string &message);
    StatementPtr parseStatement();
    StatementPtr parseCreateTable();
    StatementPtr parseInsert();
    StatementPtr parseSelect();
    std::vector<std::string> parseIdentifierList();
    std::vector<std::string> parseValueList();

    std::vector<Token> tokens_;
    size_t current_ = 0;
};

#endif // PARSER_H
