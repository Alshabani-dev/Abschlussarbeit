#ifndef PARSER_H
#define PARSER_H

#include "Ast.h"
#include "Lexer.h"

#include <string>
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    StatementList parseStatements();

private:
    const Token &peek() const;
    const Token &previous() const;
    const Token &advance();
    bool match(Token::Type type);
    const Token &consume(Token::Type type, const std::string &message);
    bool isAtEnd() const;

    StatementPtr parseStatement();
    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
    std::string parseIdentifier(const std::string &message);
    std::string parseValue();

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
};

#endif // PARSER_H
